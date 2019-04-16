#pragma once
#include "class-file.hpp"
#include "method-descriptor.hpp"
#include "method-table.hpp"
#include "opcode.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <cassert>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace fish::java {
    class Interpreter {
        using Stack = std::vector<u32>;
        using Locals = std::vector<u32>;

        struct Frame {
            Frame(std::size_t nlocals) :
            Frame(nlocals, nullptr) {
            }

            Frame(std::size_t nlocals, Frame& parent) :
            Frame(nlocals, &parent) {
            }

            void push(u32 val) {
                m_stack.push_back(val);
            }

            u32 pop() {
                u32 val = m_stack.back();
                m_stack.pop_back();
                return val;
            }

            u32& local(std::size_t i) {
                assert(i < m_locals.size());
                return m_locals[i];
            }

            Frame* parent() {
                return m_parent;
            }

            private:
            Frame(std::size_t nlocals, Frame* parent) :
            m_locals(nlocals, 0), m_parent(parent) {
            }

            Stack m_stack;
            Locals m_locals;
            Frame* m_parent = nullptr;
        };

        public:
        Interpreter(const ClassFile& cls) : m_cls(&cls) {
        }

        void run() const {
            const MethodInfo* method = m_cls->methods.main(m_cls->cpool);
            if (!method) {
                throw std::runtime_error("Could not find main() method");
            }
            const CodeInfo& code_info = method->code;
            Frame frame(code_info.max_locals);
            exec(code_info.code, frame);
        }

        private:
        const ClassFile* m_cls = nullptr;

        void exec(const CodeSeq& code, Frame& frame) const {
            for (u64 i = 0; i < code.size();) {
                const s64 inc = instr(&code[i], frame);
                if (inc == 0) {
                    return;
                }
                i += inc;
            }
            std::cerr << (
                "WARNING: Code finished executing without `return` "
                "instruction"
            ) << std::endl;
        }

        s64 instr(const u8* code, Frame& frame) const;
        s64 instr_icmp(const u8* code, Frame& frame) const;
        s64 instr_if(const u8* code, Frame& frame) const;

        template <typename T>
        static inline constexpr bool is_method_ref = std::is_convertible_v<
            std::decay_t<T>*, pool::BaseMethodRef*
        >;

        s64 instr_invokestatic(const u8* code, Frame& frame) const;
        s64 instr_invokevirtual(const u8* code, Frame& frame) const;

        void run_print(const MethodDescriptor& mdesc, Frame& frame) const {
            utils::check_print_method_descriptor(mdesc, "print()");
            print_raw(mdesc, frame);
        }

        void run_println(const MethodDescriptor& mdesc, Frame& frame) const {
            utils::check_print_method_descriptor(mdesc, "println()");
            print_raw(mdesc, frame);
            std::cout << std::endl;
        }

        void print_raw(const MethodDescriptor& mdesc, Frame& frame) const {
            if (mdesc.nargs() == 0) {
                return;
            }
            if (mdesc.arg(0) == "C") {
                std::cout << static_cast<char>(static_cast<s32>(frame.pop()));
                return;
            }
            std::cout << static_cast<s32>(frame.pop());
        }
    };
}
