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
#include "../typedefs.hpp"
#include "../utils.hpp"
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <utility>

namespace fish::java::x64 {
    inline u8 mod_rm(Register reg) {
        return static_cast<u8>(reg) % 8;
    }

    inline bool is_high_reg(Register reg) {
        return reg >= Register::r8;
    }

    class UnlinkedRel32 {
        public:
        UnlinkedRel32(
            const Instruction& inst, std::size_t base, std::size_t pos
        ) : m_inst(&inst), m_base(base), m_pos(pos) {
        }

        const Instruction& instruction() const {
            assert(m_inst);
            return *m_inst;
        }

        std::size_t base() const {
            return m_base;
        }

        std::size_t pos() const {
            return m_pos;
        }

        private:
        const Instruction* m_inst = nullptr;
        std::size_t m_base = 0;
        std::size_t m_pos = 0;
    };

    class Assembler {
        public:
        Assembler(const Program& program) : m_program(program) {
        }

        void assemble() {
            for (const Function& func : m_program.functions()) {
                assemble(func);
            }
            for (auto& unlinked : m_unlinked_rel32) {
                std::size_t abs = m_inst_map.at(&unlinked.instruction());
                std::size_t rel = abs - unlinked.base();

                // NOTE: Distance cannot be larger than (2**31 - 1).
                u32 rel32 = static_cast<u32>(rel);
                write32(rel32, &m_buf[unlinked.pos()]);
            }
        }

        std::size_t find(const Instruction& inst) const {
            return m_inst_map.at(&inst);
        }

        std::size_t find(const Function& func) const {
            auto it = func.instructions().begin();
            auto end = func.instructions().end();
            if (it == end) {
                throw std::runtime_error("Function cannot be empty");
            }
            return find(*it);
        }

        std::vector<u8>& code() {
            return m_buf;
        }

        private:
        const Program& m_program;
        std::vector<u8> m_buf;
        std::unordered_map<const Instruction*, std::size_t> m_inst_map;
        std::list<UnlinkedRel32> m_unlinked_rel32;

        void append(u8 byte) {
            m_buf.push_back(byte);
        }

        u8* pos() {
            return &m_buf[0] + m_buf.size();
        }

        void bind_rel32(const Instruction& inst) {
            m_unlinked_rel32.emplace_back(
                inst,
                pos() - &m_buf[0],
                pos() - &m_buf[0] - 4
            );
        }

        void bind_rel32(const Function& func) {
            auto it = func.instructions().begin();
            auto end = func.instructions().end();
            if (it == end) {
                throw std::runtime_error("Function cannot be empty");
            }
            bind_rel32(*it);
        }

        void assemble(const Function& func) {
            for (auto& inst : func.instructions()) {
                assemble(inst);
            }
        }

        void assemble(const Instruction& inst) {
            m_inst_map.emplace(&inst, m_buf.size());
            inst.visit([&] (auto& obj) {
                assemble(obj);
            });
        }

        void assemble(const NullaryInst& inst);
        void assemble(const UnaryInst& inst);
        void assemble(const BinaryInst& inst);
        void assemble(const Jump& inst);
        void assemble(const Call& inst);
        void assemble(const RegisterCall& inst);

        void imm32(u32 value) {
            for (std::size_t i = 0; i < 4; ++i) {
                u8 low = value & 0xff;
                value >>= 8;
                append(low);
            }
        }

        void write32(u32 value, u8* buf) {
            for (std::size_t i = 0; i < 4; ++i) {
                u8 low = value & 0xff;
                value >>= 8;
                buf[i] = low;
            }
        }

        void imm64(u64 value) {
            for (std::size_t i = 0; i < 8; ++i) {
                u8 low = value & 0xff;
                value >>= 8;
                append(low);
            }
        }

        void push(const UnaryInst& inst) {
            inst.operand().visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, Register>) {
                    if (is_high_reg(obj)) append(0x41);
                    append(0x50 + mod_rm(obj));
                }
                else if constexpr (std::is_same_v<T, Constant>) {
                    append(0x68);
                    imm32(obj.value());
                }
                else {
                    throw std::runtime_error("Unsupported operand");
                }
            });
        }

        void pop(const UnaryInst& inst) {
            auto reg = inst.operand().get<Register>();
            if (is_high_reg(reg)) append(0x41);
            append(0x58 + mod_rm(reg));
        }

        void setcc(const UnaryInst& inst, u8 opcode) {
            auto reg = inst.operand().get<Register>();
            u8 prefix = 0x40;
            prefix |= is_high_reg(reg) ? 1 : 0;
            append(prefix);
            append(0x0f);
            append(opcode);
            append(0xc0 + mod_rm(reg));
        }

        void binary_prefix(const BinaryInst& inst) {
            auto dest = inst.dest().get<Register>();
            u8 prefix = 0x48 | (is_high_reg(dest) ? 1 : 0);
            if (auto source = inst.source().get_if<Register>()) {
                prefix |= is_high_reg(*source) ? 4 : 0;
            }
            append(prefix);
        }

        struct BasicBinaryConfig {
            u8 reg_opcode;
            u8 imm_opcode;
            u8 reg_base;
            u8 imm_base;
        };

        void basic_binary(const BinaryInst& inst, BasicBinaryConfig config) {
            binary_prefix(inst);
            auto dest = inst.dest().get<Register>();

            inst.source().visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;

                if constexpr (std::is_same_v<T, Register>) {
                    append(config.reg_opcode);
                    u8 reg = config.reg_base + mod_rm(dest);
                    reg += mod_rm(obj) << 3;
                    append(reg);
                }

                else if constexpr (std::is_same_v<T, Constant>) {
                    append(config.imm_opcode);
                    append(config.imm_base + mod_rm(dest));
                    imm32(obj.value());
                }

                else {
                    throw std::runtime_error("Unsupported operand");
                }
            });
        }

        // NOTE: Only supports single-byte offset.
        void load(const BinaryInst& inst) {
            auto dest = inst.dest().get<Register>();
            auto source = inst.source().get<StackSlot>();
            append(0x48 | (is_high_reg(dest) ? 0x4 : 0));
            append(0x8b);
            append(0x45 + (mod_rm(dest) << 3));
            append(source.offset());
        }

        // NOTE: Only supports single-byte offset.
        void store(const BinaryInst& inst) {
            auto dest = inst.dest().get<StackSlot>();
            auto source = inst.source().get<Register>();
            append(0x48 | (is_high_reg(source) ? 0x4 : 0));
            append(0x89);
            append(0x45 + (mod_rm(source) << 3));
            append(dest.offset());
        }

        void mov(const BinaryInst& inst) {
            if (inst.source().get_if<StackSlot>()) {
                load(inst);
                return;
            }

            if (inst.dest().get_if<StackSlot>()) {
                store(inst);
                return;
            }

            binary_prefix(inst);
            auto dest = inst.dest().get<Register>();

            inst.source().visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, Register>) {
                    append(0x89);
                    append(0xc0 + mod_rm(dest) + (mod_rm(obj) << 3));
                }
                else if constexpr (std::is_same_v<T, Constant>) {
                    append(0xb8 + mod_rm(dest));
                    imm64(obj.value());
                }
                else {
                    throw std::runtime_error("Unsupported operand");
                }
            });
        }

        void imul(const BinaryInst& inst) {
            binary_prefix(inst);
            auto dest = inst.dest().get<Register>();
            u8 reg = 0xc0 + (mod_rm(dest) << 3);

            inst.source().visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, Register>) {
                    append(0x0f);
                    append(0xaf);
                    append(reg + mod_rm(obj));
                }
                else if constexpr (std::is_same_v<T, Constant>) {
                    append(0x69);
                    append(reg + mod_rm(dest));
                    imm32(obj.value());
                }
                else {
                    throw std::runtime_error("Unsupported operand");
                }
            });
        }

        void shift(const BinaryInst& inst, u8 reg_mask) {
            binary_prefix(inst);
            auto dest = inst.dest().get<Register>();
            u8 reg = (0xe0 + mod_rm(dest)) | reg_mask;

            inst.source().visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, Register>) {
                    append(0xd3);
                    append(reg);
                    if (obj != Register::rcx) {
                        throw std::runtime_error("Invalid shift register");
                    }
                }
                else if constexpr (std::is_same_v<T, Constant>) {
                    append(0xc1);
                    append(reg);
                    append(obj.value());
                }
                else {
                    throw std::runtime_error("Unsupported operand");
                }
            });
        }

        void test8(const BinaryInst& inst) {
            auto source = inst.source().get<Register>();
            auto dest = inst.dest().get<Register>();
            u8 prefix = 0x40;
            prefix |= is_high_reg(dest) ? 1 : 0;
            prefix |= is_high_reg(source) ? 4 : 0;
            append(prefix);
            append(0x84);
            append(0xc0 | mod_rm(dest) | (mod_rm(source) << 3));
        }
    };

    inline void Assembler::assemble(const NullaryInst& inst) {
        switch (inst.op()) {
            case NullaryInst::Op::ret: {
                append(0xc3);
                break;
            }
        }
    }

    inline void Assembler::assemble(const UnaryInst& inst) {
        switch (inst.op()) {
            case UnaryInst::Op::push: {
                push(inst);
                break;
            }

            case UnaryInst::Op::pop: {
                pop(inst);
                break;
            }

            case UnaryInst::Op::sete: {
                setcc(inst, 0x94);
                break;
            }

            case UnaryInst::Op::setne: {
                setcc(inst, 0x95);
                break;
            }

            case UnaryInst::Op::setl: {
                setcc(inst, 0x9c);
                break;
            }

            case UnaryInst::Op::setle: {
                setcc(inst, 0x9e);
                break;
            }

            case UnaryInst::Op::setg: {
                setcc(inst, 0x9f);
                break;
            }

            case UnaryInst::Op::setge: {
                setcc(inst, 0x9d);
                break;
            }
        }
    }

    inline void Assembler::assemble(const BinaryInst& inst) {
        switch (inst.op()) {
            case BinaryInst::Op::mov: {
                mov(inst);
                break;
            }

            case BinaryInst::Op::add: {
                basic_binary(inst, {0x01, 0x81, 0xc0, 0xc0});
                break;
            }

            case BinaryInst::Op::sub: {
                basic_binary(inst, {0x29, 0x81, 0xe8, 0xe8});
                break;
            }

            case BinaryInst::Op::imul: {
                imul(inst);
                break;
            }

            case BinaryInst::Op::shl: {
                shift(inst, 0x00);
                break;
            }

            case BinaryInst::Op::shr: {
                shift(inst, 0x08);
                break;
            }

            case BinaryInst::Op::cmp: {
                basic_binary(inst, {0x39, 0x81, 0xc0, 0xf8});
                break;
            }

            case BinaryInst::Op::test8: {
                test8(inst);
                break;
            }
        }
    }

    inline void Assembler::assemble(const Jump& inst) {
        switch (inst.cond()) {
            case Jump::Cond::always: {
                append(0xe9);
                imm32(0);
                bind_rel32(*inst.target());
                break;
            }

            case Jump::Cond::jz: {
                append(0x0f);
                append(0x84);
                imm32(0);
                bind_rel32(*inst.target());
                break;
            }
        }
    }

    inline void Assembler::assemble(const Call& inst) {
        append(0xe8);
        imm32(0);
        bind_rel32(inst.function());
    }

    inline void Assembler::assemble(const RegisterCall& inst) {
        auto reg = inst.reg();
        if (is_high_reg(reg)) append(0x41);
        append(0xff);
        append(0xd0 + mod_rm(reg));
    }
}
