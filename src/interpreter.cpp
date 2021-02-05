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

#include "interpreter.hpp"

namespace fish::java {
    s64 Interpreter::instr(const u8* code, Frame& frame) const {
        switch (static_cast<Opcode>(*code)) {
            case Opcode::iconst_m1:
            case Opcode::iconst_0:
            case Opcode::iconst_1:
            case Opcode::iconst_2:
            case Opcode::iconst_3:
            case Opcode::iconst_4:
            case Opcode::iconst_5: {
                frame.push(
                    static_cast<s32>(*code) -
                    static_cast<s32>(Opcode::iconst_0)
                );
                return 1;
            }

           case Opcode::iload: {
                u8 index = code[1];
                frame.push(frame.local(index));
                return 2;
            }

            case Opcode::iload_0:
            case Opcode::iload_1:
            case Opcode::iload_2:
            case Opcode::iload_3: {
                frame.push(frame.local(
                    static_cast<s32>(*code) -
                    static_cast<s32>(Opcode::iload_0)
                ));
                return 1;
            }

            case Opcode::istore: {
                u8 index = code[1];
                frame.local(index) = frame.pop();
                return 2;
            }

            case Opcode::istore_0:
            case Opcode::istore_1:
            case Opcode::istore_2:
            case Opcode::istore_3: {
                u32 val = frame.pop();
                frame.local(
                    static_cast<s32>(*code) -
                    static_cast<s32>(Opcode::istore_0)
                ) = val;
                return 1;
            }

            case Opcode::iinc: {
                u8 index = code[1];
                s8 val = static_cast<s8>(code[2]);
                frame.local(index) += val;
                return 3;
            }

            case Opcode::iadd: {
                u32 val = frame.pop();
                frame.push(frame.pop() + val);
                return 1;
            }

            case Opcode::isub: {
                u32 val = frame.pop();
                frame.push(frame.pop() - val);
                return 1;
            }

            case Opcode::imul: {
                u32 val = frame.pop();
                frame.push(frame.pop() * val);
                return 1;
            }

            case Opcode::ishl: {
                u32 amount = frame.pop() & 0b11111;
                frame.push(frame.pop() << amount);
                return 1;
            }

            case Opcode::ishr: {
                u32 amount = frame.pop() & 0b11111;
                u32 val = frame.pop();
                u32 result = val >>= amount;
                if (val & (1 << (32 - 1))) {
                    result |= ((1 << amount) - 1) << (32 - amount);
                }
                frame.push(result);
                return 1;
            }

            case Opcode::if_icmpeq:
            case Opcode::if_icmpne:
            case Opcode::if_icmpgt:
            case Opcode::if_icmpge:
            case Opcode::if_icmplt:
            case Opcode::if_icmple: {
                return instr_icmp(code, frame);
            }

            case Opcode::ifeq:
            case Opcode::ifne:
            case Opcode::ifgt:
            case Opcode::ifge:
            case Opcode::iflt:
            case Opcode::ifle: {
                return instr_if(code, frame);
            }

            case Opcode::Goto: {
                return static_cast<s16>(code[1] << 8 | code[2]);
            }

            case Opcode::bipush: {
                frame.push(static_cast<s8>(code[1]));
                return 2;
            }

            case Opcode::sipush: {
                frame.push(static_cast<s16>(code[1] << 8 | code[2]));
                return 3;
            }

            case Opcode::invokestatic: {
                return instr_invokestatic(code, frame);
            }

            case Opcode::invokevirtual: {
                return instr_invokevirtual(code, frame);
            }

            case Opcode::Return: {
                return 0;
            }

            case Opcode::ireturn: {
                u64 val = frame.pop();
                if (Frame* parent = frame.parent()) {
                    parent->push(val);
                }
                return 0;
            }

            case Opcode::getstatic: {
                // NOTE: Ignoring object
                frame.push(0);
                return 3;
            }

            case Opcode::pop: {
                frame.pop();
                return 1;
            }

            default: {
                std::ostringstream msg;
                msg << "Unsupported opcode: 0x";
                msg << std::hex << static_cast<int>(*code);
                throw std::runtime_error(msg.str());
            }
        }
    }

    s64 Interpreter::instr_icmp(const u8* code, Frame& frame) const {
        const s32 y = static_cast<s32>(frame.pop());
        const s32 x = static_cast<s32>(frame.pop());
        bool branch = false;
        switch (static_cast<Opcode>(*code)) {
            case Opcode::if_icmpeq: {
                branch = x == y;
                break;
            }
            case Opcode::if_icmpne: {
                branch = x != y;
                break;
            }
            case Opcode::if_icmpgt: {
                branch = x > y;
                break;
            }
            case Opcode::if_icmpge: {
                branch = x >= y;
                break;
            }
            case Opcode::if_icmplt: {
                branch = x < y;
                break;
            }
            case Opcode::if_icmple: {
                branch = x <= y;
                break;
            }
            default: {
                std::ostringstream msg;
                msg << "Invalid `if_icmp` opcode: 0x";
                msg << std::hex << static_cast<int>(*code);
                throw std::runtime_error(msg.str());
            }
        }
        if (branch) {
            return static_cast<s16>(code[1] << 8 | code[2]);
        }
        return 3;
    }

    s64 Interpreter::instr_if(const u8* code, Frame& frame) const {
        const s32 x = static_cast<s32>(frame.pop());
        bool branch = false;
        switch (static_cast<Opcode>(*code)) {
            case Opcode::ifeq: {
                branch = x == 0;
                break;
            }
            case Opcode::ifne: {
                branch = x != 0;
                break;
            }
            case Opcode::ifgt: {
                branch = x > 0;
                break;
            }
            case Opcode::ifge: {
                branch = x >= 0;
                break;
            }
            case Opcode::iflt: {
                branch = x < 0;
                break;
            }
            case Opcode::ifle: {
                branch = x <= 0;
                break;
            }
            default: {
                std::ostringstream msg;
                msg << "Invalid `if` opcode: 0x";
                msg << std::hex << static_cast<int>(*code);
                throw std::runtime_error(msg.str());
            }
        }
        if (branch) {
            return static_cast<s16>(code[1] << 8 | code[2]);
        }
        return 3;
    }

    // NOTE: Supports only static methods in the same class, arguments
    // must be integers, and the return type must be int or void.
    s64 Interpreter::instr_invokestatic(const u8* code, Frame& frame) const {
        const u16 index = code[1] << 8 | code[2];
        const ConstantPool& cpool = m_cls->cpool;

        auto& name_and_type = cpool[index].visit(
            [&] (auto& obj) -> const pool::NameAndType& {
                if constexpr (!is_method_ref<decltype(obj)>) {
                    throw std::runtime_error(
                        "Expected method entry in constant pool"
                    );
                }
                else if (obj.class_ref_index != m_cls->self_index) {
                    throw std::runtime_error(
                        "Cannot call method of other class"
                    );
                }
                else {
                    return cpool.get<pool::NameAndType>(
                        obj.name_type_index
                    );
                }
            }
        );

        const MethodTable& methods = m_cls->methods;
        const MethodInfo* method = methods.find(name_and_type);
        if (!method) {
            throw std::runtime_error("No such method");
        }

        const CodeInfo& code_info = method->code;
        Frame new_frame(code_info.max_locals, frame);
        MethodDescriptor mdesc = method->descriptor(cpool);
        const std::size_t nargs = mdesc.nargs();

        // NOTE: Assuming only 32-bit arguments
        for (std::size_t i = nargs; i > 0; --i) {
            new_frame.local(i - 1) = frame.pop();
        }
        exec(code_info.code, new_frame);
        return 3;
    }

    // NOTE: Supports only print() and println() with int, char, or void arg.
    s64 Interpreter::instr_invokevirtual(const u8* code, Frame& frame) const {
        const u16 index = code[1] << 8 | code[2];
        const ConstantPool& cpool = m_cls->cpool;

        auto& name_and_type = cpool[index].visit(
            [&] (auto& obj) -> const pool::NameAndType& {
                if constexpr (!is_method_ref<decltype(obj)>) {
                    throw std::runtime_error(
                        "Expected method entry in constant pool"
                    );
                }
                else {
                    return cpool.get<pool::NameAndType>(
                        obj.name_type_index
                    );
                }
            }
        );

        auto& name = cpool.get<pool::UTF8>(name_and_type.name_index).str;
        auto& sig = cpool.get<pool::UTF8>(name_and_type.desc_index).str;
        MethodDescriptor mdesc(sig);

        if (name == "print") {
            run_print(mdesc, frame);
        }
        else if (name == "println") {
            run_println(mdesc, frame);
        }
        else {
            std::ostringstream msg;
            msg << "Unsupported virtual method: " << name;
            throw std::runtime_error(msg.str());
        }

        frame.pop();  // Object ref
        return 3;
    }
}
