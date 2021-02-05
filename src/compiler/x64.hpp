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
#include "../typedefs.hpp"
#include "../utils.hpp"
#include <cassert>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <variant>

namespace fish::java::x64 {
    class Instruction;
    class Function;

    using InstructionList = std::list<Instruction>;
    using InstructionIterator = InstructionList::iterator;
    using ConstInstructionIterator = InstructionList::const_iterator;

    using ssa::Constant;
    using ssa::ArithmeticOperator;

    class NullaryInst;
    class UnaryInst;
    class BinaryInst;
    class Jump;
    class Call;
    class RegisterCall;

    enum class Register {
        rax,
        rcx,
        rdx,
        rbx,
        rsp,
        rbp,
        rsi,
        rdi,
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15,
    };

    class StackSlot {
        public:
        StackSlot(s64 offset) : m_offset(offset) {
        }

        s64 offset() const {
            return m_offset;
        }

        private:
        s64 m_offset = 0;
    };
}

namespace fish::java::x64::variants {
    using Instruction = std::variant<
        NullaryInst,
        UnaryInst,
        BinaryInst,
        Jump,
        Call,
        RegisterCall
    >;

    using Operand = std::variant<
        Constant,
        Register,
        StackSlot
    >;
}

namespace fish::java::x64 {
    class Operand : private utils::VariantWrapper<variants::Operand> {
        public:
        using VariantWrapper::VariantWrapper;
        using VariantWrapper::visit;
        using VariantWrapper::get;
        using VariantWrapper::get_if;
    };

    class NullaryInst {
        public:
        enum class Op {
            ret,
        };

        NullaryInst(Op op) : m_op(op) {
        }

        Op& op() {
            return m_op;
        }

        Op op() const {
            return m_op;
        }

        private:
        Op m_op = {};
    };

    class UnaryInst {
        public:
        enum class Op {
            push,
            pop,
            sete,
            setne,
            setl,
            setle,
            setg,
            setge,
        };

        template <typename Oper>
        UnaryInst(Op op, Oper&& operand) :
        m_op(op), m_operand(std::forward<Oper>(operand)) {
        }

        Op& op() {
            return m_op;
        }

        Op op() const {
            return m_op;
        }

        auto& operand() {
            return m_operand;
        }

        auto& operand() const {
            return m_operand;
        }

        private:
        Op m_op = {};
        Operand m_operand;
    };

    class BinaryInst {
        public:
        enum class Op {
            mov,
            add,
            sub,
            imul,
            shl,
            shr,
            cmp,
            test8,
        };

        template <typename Dest, typename Source>
        BinaryInst(Op op, Dest&& dest, Source&& source) :
        m_op(op),
        m_dest(std::forward<Dest>(dest)),
        m_source(std::forward<Source>(source)) {
        }

        Op& op() {
            return m_op;
        }

        Op op() const {
            return m_op;
        }

        auto& dest() {
            return m_dest;
        }

        auto& dest() const {
            return m_dest;
        }

        auto& source() {
            return m_source;
        }

        auto& source() const {
            return m_source;
        }

        private:
        Op m_op = {};
        Operand m_dest;
        Operand m_source;
    };

    class Jump {
        public:
        enum class Cond {
            always,
            jz,
        };

        Jump(Cond cond = Cond::always) : m_cond(cond) {
        }

        Jump(Cond cond, InstructionIterator target) :
        m_cond(cond), m_target(target) {
        }

        Cond& cond() {
            return m_cond;
        }

        Cond cond() const {
            return m_cond;
        }

        auto& target() {
            assert(m_target);
            return *m_target;
        }

        const auto& target() const {
            return const_cast<Jump&>(*this).target();
        }

        auto& target(std::nullopt_t) {
            return m_target;
        }

        private:
        Cond m_cond = {};
        std::optional<InstructionIterator> m_target;
    };

    class Call {
        public:
        Call(Function& function) : m_function(&function) {
        }

        Function& function() {
            assert(m_function);
            return *m_function;
        }

        const Function& function() const {
            return const_cast<Call&>(*this).function();
        }

        private:
        Function* m_function = nullptr;
    };

    class RegisterCall {
        public:
        RegisterCall(Register reg) : m_reg(reg) {
        }

        Register& reg() {
            return m_reg;
        }

        Register reg() const {
            return m_reg;
        }

        private:
        Register m_reg;
    };

    class Instruction : private utils::VariantWrapper<variants::Instruction> {
        public:
        using VariantWrapper::VariantWrapper;
        using VariantWrapper::visit;
        using VariantWrapper::get;
        using VariantWrapper::get_if;
    };

    class InstructionSequence {
        public:
        using iterator = InstructionIterator;
        using const_iterator = ConstInstructionIterator;

        auto begin() {
            return m_instructions.begin();
        }

        auto begin() const {
            return m_instructions.begin();
        }

        auto end() {
            return m_instructions.end();
        }

        auto end() const {
            return m_instructions.end();
        }

        template <typename T>
        auto append(T&& instruction) {
            return m_instructions.emplace(
                m_instructions.end(), std::forward<T>(instruction)
            );
        }

        private:
        InstructionList m_instructions;
    };

    class Function {
        public:
        Function(std::string name) : m_name(std::move(name)) {
        }

        auto& instructions() {
            return m_instructions;
        }

        auto& instructions() const {
            return m_instructions;
        }

        std::string& name() {
            return m_name;
        }

        const std::string& name() const {
            return m_name;
        }

        private:
        InstructionSequence m_instructions;
        std::string m_name;
    };

    class FunctionSet {
        using List = std::list<Function>;

        public:
        using iterator = List::iterator;

        auto begin() {
            return m_functions.begin();
        }

        auto begin() const {
            return m_functions.begin();
        }

        auto end() {
            return m_functions.end();
        }

        auto end() const {
            return m_functions.end();
        }

        auto add(const Function& function) {
            return emplace(function);
        }

        auto add(Function&& function) {
            return emplace(std::move(function));
        }

        private:
        List m_functions;

        template <typename... Args>
        iterator emplace(Args&&... args) {
            return m_functions.emplace(
                m_functions.end(), std::forward<Args>(args)...
            );
        }
    };

    class Program {
        public:
        Program() = default;

        auto& functions() {
            return m_functions;
        }

        auto& functions() const {
            return m_functions;
        }

        private:
        FunctionSet m_functions;
    };
}
