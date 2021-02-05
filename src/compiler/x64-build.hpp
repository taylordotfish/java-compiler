/*
 * Copyright (C) 2019 taylor.fish <contact@taylor.fish>
 *
 * This file is part of java-compiler.
 *
 * java-compiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * java-compiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with java-compiler. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "x64.hpp"
#include "x64-alloc.hpp"
#include "x64-builtins.hpp"
#include "../utils.hpp"
#include <list>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace fish::java::x64 {
    class ProgramBuilder {
        public:
        ProgramBuilder(Program& program, ssa::Program& ssa_prog) :
        m_program(program), m_ssa_prog(ssa_prog) {
            for (ssa::Function& ssa_func : m_ssa_prog.functions()) {
                auto it = m_program.functions().add(Function(ssa_func.name()));
                Function& func = *it;
                m_func_map.emplace(&ssa_func, &func);
            }
        }

        void build();

        protected:
        friend class FunctionBuilder;

        Function& function(const ssa::Function& ssa_func) {
            auto it = m_func_map.find(const_cast<ssa::Function*>(&ssa_func));
            assert(it != m_func_map.end());
            return *it->second;
        }

        private:
        Program& m_program;
        ssa::Program& m_ssa_prog;
        std::unordered_map<ssa::Function*, Function*> m_func_map;
    };

    class FunctionBuilder {
        using InstIter = InstructionIterator;
        using OptInstIter = std::optional<InstructionIterator>;

        public:
        FunctionBuilder(
            ProgramBuilder& parent, Function& function,
            ssa::Function& ssa_func
        ) :
        m_parent(parent),
        m_func(function),
        m_ssa_func(ssa_func) {
            RegisterAllocator allocator(ssa_func);
            allocator.allocate();
            m_regs = std::move(allocator.regs());
            m_live_var_map = std::move(allocator.live_var_map());
        }

        void build() {
            for (ssa::BasicBlock& ssa_block : m_ssa_func.blocks()) {
                build(ssa_block);
            }
            for (auto& pair : m_unlinked) {
                auto block = pair.first;
                auto& inst = *pair.second;
                inst = m_block_map.at(block);
            }
        }

        private:
        ProgramBuilder& m_parent;
        Function& m_func;
        ssa::Function& m_ssa_func;
        RegMap m_regs;
        LiveVarMap m_live_var_map;

        std::unordered_map<const ssa::BasicBlock*, InstIter> m_block_map;
        std::list<std::pair<const ssa::BasicBlock*, OptInstIter*>> m_unlinked;

        const ssa::BasicBlock* m_block = nullptr;
        bool m_prologue_done = false;

        Function& function(const ssa::Function& ssa_func) {
            return m_parent.function(ssa_func);
        }

        template <typename T>
        InstructionIterator append(T&& inst) {
            auto it = m_func.instructions().append(std::forward<T>(inst));
            if (m_block) {
                m_block_map.emplace(m_block, it);
                m_block = nullptr;
            }
            return it;
        }

        Register reg(ssa::InstructionIterator ssa_inst) const {
            return m_regs.at(ssa_inst);
        }

        std::optional<Register>
        reg_opt(ssa::InstructionIterator ssa_inst) const {
            auto it = m_regs.find(ssa_inst);
            if (it != m_regs.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        void bind(OptInstIter& inst, const ssa::BasicBlock& block) {
            auto it = m_block_map.find(&block);
            if (it == m_block_map.end()) {
                m_unlinked.emplace_back(&block, &inst);
            } else {
                inst = it->second;
            }
        }

        void ensure_prologue() {
            if (!m_prologue_done) {
                prologue();
            }
        }

        u64 sspace() const {
            auto nslots = m_ssa_func.stack_slots();
            auto nargs = m_ssa_func.nargs();
            // Ensure 16-byte stack alignment
            return 8 * (nslots + (nslots + nargs) % 2);
        }

        void prologue() {
            append(UnaryInst(UnaryInst::Op::push, Register::rbp));
            append(BinaryInst(
                BinaryInst::Op::mov, Register::rbp, Register::rsp
            ));
            append(BinaryInst(
                BinaryInst::Op::sub, Register::rsp, Constant(sspace())
            ));
            m_prologue_done = true;
        }

        void epilogue() {
            append(BinaryInst(
                BinaryInst::Op::add, Register::rsp, Constant(sspace())
            ));
            append(UnaryInst(UnaryInst::Op::pop, Register::rbp));
        }

        auto save_registers(ssa::InstructionIterator inst) {
            std::optional<Register> reg = reg_opt(inst);
            std::list<Register> saved;
            auto ptr = static_cast<const void*>(&*inst);

            for (auto live : m_live_var_map[ptr]) {
                std::optional<Register> live_reg = reg_opt(live);
                if (!live_reg) continue;
                if (reg && *reg == *live_reg) continue;
                saved.push_front(*live_reg);
                append(UnaryInst(UnaryInst::Op::push, *live_reg));
            }

            // Ensure 16-byte stack alignment
            if (saved.size() % 2 == 1) {
                append(BinaryInst(
                    BinaryInst::Op::sub, Register::rsp, Constant(8)
                ));
            }
            return saved;
        }

        void restore_registers(const std::list<Register>& saved) {
            // Ensure 16-byte stack alignment
            if (saved.size() % 2 == 1) {
                append(BinaryInst(
                    BinaryInst::Op::add, Register::rsp, Constant(8)
                ));
            }
            for (auto reg : saved) {
                append(UnaryInst(UnaryInst::Op::pop, reg));
            }
        }

        Operand operand(const ssa::Value& ssa_value) const;
        void build(ssa::BasicBlock& ssa_block);
        void build(ssa::InstructionIterator& ssa_inst);
        void build_block_end(ssa::BasicBlock& ssa_block);
        void build_shift(const ssa::BinaryOperation& inst, Register dest);
        void build_phi_transfers(ssa::BasicBlock& ssa_block);
    };

    inline void ProgramBuilder::build() {
        for (auto& pair : m_func_map) {
            ssa::Function& ssa_func = *pair.first;
            Function& func = *pair.second;
            FunctionBuilder builder(*this, func, ssa_func);
            builder.build();
        }
    }

    inline Operand
    FunctionBuilder::operand(const ssa::Value& ssa_value) const {
        return ssa_value.visit([&] (auto& obj) -> Operand {
            using T = std::decay_t<decltype(obj)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                throw std::runtime_error("Unexpected empty ssa::Value");
            }

            else if constexpr (std::is_same_v<T, ssa::Constant>) {
                return Operand(obj);
            }

            else if constexpr (std::is_same_v<T, ssa::InstructionIterator>) {
                return Operand(reg(obj));
            }

            else {
                static_assert(utils::always_false<T>);
            }
        });
    }

    inline void FunctionBuilder::build(ssa::BasicBlock& ssa_block) {
        m_block = &ssa_block;
        decltype(auto) instructions = ssa_block.instructions();
        auto it = instructions.begin();
        auto end = instructions.end();
        for (; it != end; ++it) {
            build(it);
        }
        build_block_end(ssa_block);
    }

    inline void FunctionBuilder::build(ssa::InstructionIterator& ssa_inst) {
        ensure_prologue();
        std::optional<Register> dest = reg_opt(ssa_inst);
        ssa_inst->visit([&] (auto& obj) {
            using T = std::decay_t<decltype(obj)>;

            if constexpr (std::is_same_v<T, ssa::Move>) {
                if (!dest) return;
                append(BinaryInst(
                    BinaryInst::Op::mov, *dest, operand(obj.value())
                ));
            }

            else if constexpr (std::is_same_v<T, ssa::BinaryOperation>) {
                if (!dest) return;
                switch (obj.op()) {
                    case ssa::BinaryOperation::Op::shl:
                    case ssa::BinaryOperation::Op::shr: {
                        build_shift(obj, *dest);
                        return;
                    }
                    default:;
                }

                auto right = operand(obj.right());
                auto right_reg = right.template get_if<Register>();
                if (right_reg && *right_reg == *dest) {
                    append(BinaryInst(
                        BinaryInst::Op::mov, Register::rcx, right
                    ));
                    right = Operand(Register::rcx);
                }

                // Move LHS to dest if needed.
                auto left = operand(obj.left());
                auto left_reg = left.template get_if<Register>();
                if (left_reg && *left_reg != *dest) {
                    append(BinaryInst(BinaryInst::Op::mov, *dest, left));
                }

                switch (obj.op()) {
                    case ssa::BinaryOperation::Op::add: {
                        append(BinaryInst(BinaryInst::Op::add, *dest, right));
                        break;
                    }
                    case ssa::BinaryOperation::Op::sub: {
                        append(BinaryInst(BinaryInst::Op::sub, *dest, right));
                        break;
                    }
                    case ssa::BinaryOperation::Op::mul: {
                        append(BinaryInst(BinaryInst::Op::imul, *dest, right));
                        break;
                    }
                    default:;
                }
            }

            else if constexpr (std::is_same_v<T, ssa::Comparison>) {
                if (!dest) return;

                // Move LHS to dest if needed.
                auto left = operand(obj.left());
                auto left_reg = left.template get_if<Register>();
                if (!left_reg) {
                    append(BinaryInst(BinaryInst::Op::mov, *dest, left));
                    left = Operand(*dest);
                }

                append(BinaryInst(
                    BinaryInst::Op::cmp, left, operand(obj.right())
                ));

                switch (obj.op()) {
                    case ssa::Comparison::Op::eq: {
                        append(UnaryInst(UnaryInst::Op::sete, *dest));
                        break;
                    }

                    case ssa::Comparison::Op::ne: {
                        append(UnaryInst(UnaryInst::Op::setne, *dest));
                        break;
                    }

                    case ssa::Comparison::Op::lt: {
                        append(UnaryInst(UnaryInst::Op::setl, *dest));
                        break;
                    }

                    case ssa::Comparison::Op::le: {
                        append(UnaryInst(UnaryInst::Op::setle, *dest));
                        break;
                    }

                    case ssa::Comparison::Op::gt: {
                        append(UnaryInst(UnaryInst::Op::setg, *dest));
                        break;
                    }

                    case ssa::Comparison::Op::ge: {
                        append(UnaryInst(UnaryInst::Op::setge, *dest));
                        break;
                    }
                }
            }

            else if constexpr (std::is_same_v<T, ssa::FunctionCall>) {
                auto saved = save_registers(ssa_inst);
                for (auto& arg : obj.args()) {
                    append(UnaryInst(UnaryInst::Op::push, operand(arg)));
                }
                append(Call(function(obj.function())));
                append(BinaryInst(
                    BinaryInst::Op::add, Register::rsp,
                    Constant(obj.args().size() * 8)
                ));
                if (obj.function().nreturn() > 0 && dest) {
                    append(BinaryInst(
                        BinaryInst::Op::mov, *dest, Register::rax
                    ));
                }
                restore_registers(saved);
            }

            else if constexpr (std::is_same_v<T, ssa::StandardCall>) {
                u64 address = 0;
                switch (obj.kind()) {
                    case ssa::StandardCall::Kind::print_char: {
                        address = (u64)(&fish_java_x64_print_char);
                        break;
                    }
                    case ssa::StandardCall::Kind::print_int: {
                        address = (u64)(&fish_java_x64_print_int);
                        break;
                    }
                    case ssa::StandardCall::Kind::println_void: {
                        address = (u64)(&fish_java_x64_println_void);
                        break;
                    }
                    case ssa::StandardCall::Kind::println_char: {
                        address = (u64)(&fish_java_x64_println_char);
                        break;
                    }
                    case ssa::StandardCall::Kind::println_int: {
                        address = (u64)(&fish_java_x64_println_int);
                        break;
                    }
                }

                auto saved = save_registers(ssa_inst);
                for (auto& arg : obj.args()) {
                    append(UnaryInst(UnaryInst::Op::push, operand(arg)));
                }
                append(BinaryInst(
                    BinaryInst::Op::mov, Register::rcx, Constant(address)
                ));
                append(RegisterCall(Register::rcx));
                append(BinaryInst(
                    BinaryInst::Op::add, Register::rsp,
                    Constant(obj.args().size() * 8)
                ));
                restore_registers(saved);
            }

            else if constexpr (std::is_same_v<T, ssa::Phi>) {
            }

            else if constexpr (std::is_same_v<T, ssa::Load>) {
                append(BinaryInst(
                    BinaryInst::Op::mov, dest.value(),
                    StackSlot(8 * (-static_cast<s64>(obj.index()) - 1))
                ));
            }

            else if constexpr (std::is_same_v<T, ssa::Store>) {
                append(BinaryInst(
                    BinaryInst::Op::mov,
                    StackSlot(8 * (-static_cast<s64>(obj.index()) - 1)),
                    operand(obj.value())
                ));
            }

            else if constexpr (std::is_same_v<T, ssa::LoadArgument>) {
                if (!dest) return;
                append(BinaryInst(
                    BinaryInst::Op::mov, *dest,
                    StackSlot(8 * (m_ssa_func.nargs() - 1 + 2 - obj.index()))
                ));
            }

            else {
                static_assert(utils::always_false<T>);
            }
        });
    }

    inline void FunctionBuilder::build_block_end(ssa::BasicBlock& ssa_block) {
        ensure_prologue();
        ssa_block.terminator().visit([&] (auto& obj) {
            using T = std::decay_t<decltype(obj)>;

            if constexpr (std::is_same_v<T, ssa::UnconditionalBranch>) {
                build_phi_transfers(ssa_block);
                auto it = append(Jump());
                auto& jump = it->get<Jump>();
                bind(jump.target(std::nullopt), obj.target());
            }

            else if constexpr (std::is_same_v<T, ssa::Branch>) {
                auto oper = operand(obj.cond());
                append(BinaryInst(BinaryInst::Op::mov, Register::rcx, oper));

                build_phi_transfers(ssa_block);
                append(BinaryInst(
                    BinaryInst::Op::test8, Register::rcx, Register::rcx
                ));

                auto it1 = append(Jump(Jump::Cond::jz));
                auto& jump1 = it1->get<Jump>();
                bind(jump1.target(std::nullopt), obj.no());

                auto it2 = append(Jump());
                auto& jump2 = it2->get<Jump>();
                bind(jump2.target(std::nullopt), obj.yes());
            }

            else if constexpr (std::is_same_v<T, ssa::ReturnVoid>) {
                epilogue();
                append(NullaryInst(NullaryInst::Op::ret));
            }

            else if constexpr (std::is_same_v<T, ssa::Return>) {
                append(BinaryInst(
                    BinaryInst::Op::mov, Register::rax, operand(obj.value())
                ));
                epilogue();
                append(NullaryInst(NullaryInst::Op::ret));
            }

            else {
                static_assert(utils::always_false<T>);
            }
        });
    }

    inline void FunctionBuilder::
    build_shift(const ssa::BinaryOperation& inst, Register dest) {
        auto right = operand(inst.right());
        if (right.get_if<Register>()) {
            append(BinaryInst(BinaryInst::Op::mov, Register::rcx, right));
            right = Operand(Register::rcx);
        }

        // Move LHS to dest if needed.
        auto left = operand(inst.left());
        auto left_reg = left.get_if<Register>();
        if (left_reg && *left_reg != dest) {
            append(BinaryInst(BinaryInst::Op::mov, dest, left));
        }

        switch (inst.op()) {
            case ssa::BinaryOperation::Op::shl: {
                append(BinaryInst(BinaryInst::Op::shl, dest, right));
                break;
            }
            case ssa::BinaryOperation::Op::shr: {
                append(BinaryInst(BinaryInst::Op::shr, dest, right));
                break;
            }
            default:;
        }
    }

    inline void FunctionBuilder::
    build_phi_transfers(ssa::BasicBlock& ssa_block) {
        for (ssa::BasicBlock* succ : ssa_block.successors()) {
            for (auto& [phi, input] : succ->phis(ssa_block)) {
                if (std::optional<Register> reg = reg_opt(phi)) {
                    append(BinaryInst(
                        BinaryInst::Op::mov, *reg, operand(*input)
                    ));
                }
            }
        }
    }
}
