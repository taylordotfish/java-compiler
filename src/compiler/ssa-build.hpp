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
#include "dominators.hpp"
#include "java.hpp"
#include "ssa.hpp"
#include "../constant-pool.hpp"
#include "../code-info.hpp"
#include "../method-info.hpp"
#include "../opcode.hpp"
#include "../typedefs.hpp"
#include <cassert>
#include <cstddef>
#include <map>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <variant>

namespace fish::java::ssa {
    class ProgramBuilder {
        public:
        ProgramBuilder(Program& program, const java::Program& j_prog) :
        m_program(program), m_j_prog(j_prog) {
            for (const java::Function& j_func : m_j_prog.functions()) {
                auto it = m_program.functions().add(Function(
                    j_func.nargs(), j_func.nreturn(), j_func.name()
                ));
                Function& func = *it;
                m_func_map.emplace(&j_func, &func);
            }
        }

        void build();

        protected:
        friend class FunctionBuilder;

        Function& function(const java::Function& j_func) {
            auto it = m_func_map.find(&j_func);
            assert(it != m_func_map.end());
            return *it->second;
        }

        private:
        Program& m_program;
        const java::Program& m_j_prog;
        std::unordered_map<const java::Function*, Function*> m_func_map;
    };

    class FunctionBuilder {
        public:
        FunctionBuilder(
            ProgramBuilder& parent, Function& function,
            const java::Function& j_func
        ) :
        m_parent(parent),
        m_func(function),
        m_j_func(j_func) {
            auto blocks = m_func.blocks();
            BasicBlock& entry = *blocks.append(BasicBlock());
            BasicBlock& first = block(m_j_func.instructions().begin());
            entry.terminate(UnconditionalBranch(first));

            auto& defs = m_defs[&entry];
            for (std::size_t i = 0; i < m_func.nargs(); ++i) {
                auto it = entry.instructions().append(LoadArgument(i));
                defs.emplace(Variable(Variable::locals, i), it);
            }
        }

        void build();

        protected:
        friend class BlockBuilder;

        BasicBlock& block(java::ConstInstructionIterator j_inst) {
            auto it = m_block_map.find(&*j_inst);
            if (it == m_block_map.end()) {
                auto& block = *m_func.blocks().append(BasicBlock());
                m_block_map.emplace(&*j_inst, &block);
                return block;
            }
            return *it->second;
        }

        Function& function(const java::Function& j_func) {
            return m_parent.function(j_func);
        }

        auto& unlinked(BasicBlock& block) {
            return m_unlinked[&block];
        }

        auto& defs(BasicBlock& block) {
            return m_defs[&block];
        }

        private:
        ProgramBuilder& m_parent;
        Function& m_func;
        const java::Function& m_j_func;

        DefMap m_defs;
        UnlinkedMap m_unlinked;
        std::unordered_map<const java::Instruction*, BasicBlock*> m_block_map;
    };

    inline void ProgramBuilder::build() {
        for (const auto& pair : m_func_map) {
            const java::Function& j_func = *pair.first;
            Function& func = *pair.second;
            FunctionBuilder builder(*this, func, j_func);
            builder.build();
        }
    }

    class BlockBuilder {
        public:
        BlockBuilder(
            FunctionBuilder& parent, BasicBlock& block,
            java::ConstInstructionIterator j_inst
        ) :
        m_parent(parent),
        m_block(block),
        m_j_inst(j_inst),
        m_unlinked(parent.unlinked(block)),
        m_defs(parent.defs(block)) {
        }

        java::ConstInstructionIterator build() {
            while (!build_instruction()) {
                ++m_j_inst;
                ++m_index;
            }
            return m_j_inst;
        }

        private:
        FunctionBuilder& m_parent;
        BasicBlock& m_block;
        java::ConstInstructionIterator m_j_inst;

        std::list<UnlinkedValue>& m_unlinked;
        std::map<Variable, Value>& m_defs;
        std::size_t m_index = 0;

        template <typename T>
        InstructionIterator append(T&& inst) {
            return m_block.instructions().append(std::forward<T>(inst));
        }

        template <typename T>
        Terminator& terminate(T&& term) {
            return m_block.terminate(std::forward<T>(term));
        }

        void bind(Value& dest, const java::Value& source) {
            source.visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;

                if constexpr (std::is_same_v<T, java::Constant>) {
                    dest = Value(obj);
                }

                else if constexpr (std::is_same_v<T, Variable>) {
                    auto it = m_defs.find(obj);
                    if (it == m_defs.end()) {
                        m_unlinked.emplace_back(obj, dest);
                    } else {
                        dest = it->second;
                    }
                }

                else {
                    static_assert(utils::always_false<T>);
                }
            });
        }

        template <typename T>
        void define(Variable var, T&& value) {
            m_defs.insert_or_assign(var, Value(std::forward<T>(value)));
        }

        BasicBlock& block(java::ConstInstructionIterator j_inst) {
            return m_parent.block(j_inst);
        }

        Function& function(const java::Function& j_func) {
            return m_parent.function(j_func);
        }

        bool build_instruction();
    };

    inline void FunctionBuilder::build() {
        decltype(auto) instructions = m_j_func.instructions();
        java::ConstInstructionIterator j_inst = instructions.begin();
        java::ConstInstructionIterator end = instructions.end();

        while (j_inst != end) {
            BasicBlock& block = this->block(j_inst);
            BlockBuilder builder(*this, block, j_inst);
            j_inst = builder.build();
        }

        PhiFixer fixer(m_func, m_defs);
        fixer.fix();

        for (auto& [block, unlinked] : m_unlinked) {
            auto& links = fixer.links(*block);
            for (auto& entry : unlinked) {
                entry.value() = links.at(entry.var());
            }
        }
    }

    inline bool BlockBuilder::build_instruction() {
        if (m_index > 0 && m_j_inst->flags().target()) {
            terminate(UnconditionalBranch(block(m_j_inst)));
            return true;
        }

        return m_j_inst->visit([&] ([[maybe_unused]] auto& j_inst) {
            using T = std::decay_t<decltype(j_inst)>;

            if constexpr (std::is_same_v<T, java::Move>) {
                auto it = append(Move());
                Move& move = it->get<Move>();
                bind(move.value(), j_inst.source());
                define(j_inst.dest(), it);
                return false;
            }

            else if constexpr (std::is_same_v<T, java::BinaryOperation>) {
                auto it = append(BinaryOperation(j_inst.op()));
                BinaryOperation& inst = it->get<BinaryOperation>();
                bind(inst.left(), j_inst.left());
                bind(inst.right(), j_inst.right());
                define(j_inst.dest(), it);
                return false;
            }

            else if constexpr (std::is_same_v<T, java::Branch>) {
                auto cmp_it = append(Comparison(j_inst.op()));
                Comparison& cmp = cmp_it->get<Comparison>();
                bind(cmp.left(), j_inst.left());
                bind(cmp.right(), j_inst.right());

                java::ConstInstructionIterator yes_pos = j_inst.target();
                java::ConstInstructionIterator no_pos = ++m_j_inst;
                terminate(Branch(cmp_it, block(yes_pos), block(no_pos)));
                return true;
            }

            else if constexpr (std::is_same_v<T, java::UnconditionalBranch>) {
                java::ConstInstructionIterator pos = j_inst.target();
                terminate(UnconditionalBranch(block(pos)));
                ++m_j_inst;
                return true;
            }

            else if constexpr (std::is_same_v<T, java::Return>) {
                Terminator& term = terminate(Return());
                Return& ret = term.get<Return>();
                bind(ret.value(), j_inst.value());
                ++m_j_inst;
                return true;
            }

            else if constexpr (std::is_same_v<T, java::ReturnVoid>) {
                terminate(ReturnVoid());
                ++m_j_inst;
                return true;
            }

            else if constexpr (std::is_same_v<T, java::FunctionCall>) {
                auto it = append(FunctionCall(function(j_inst.function())));
                FunctionCall& call = it->get<FunctionCall>();
                for (const java::Value& arg : j_inst.args()) {
                    bind(call.args().emplace_back(), arg);
                }
                if (auto& dest = j_inst.dest()) {
                    define(*dest, it);
                }
                return false;
            }

            else if constexpr (std::is_same_v<T, java::StandardCall>) {
                auto it = append(StandardCall(j_inst.kind()));
                StandardCall& call = it->get<StandardCall>();
                for (const java::Value& arg : j_inst.args()) {
                    bind(call.args().emplace_back(), arg);
                }
                return false;
            }

            else {
                static_assert(utils::always_false<T>);
                return false;
            }
        });
    }
}
