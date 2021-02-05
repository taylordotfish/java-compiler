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
#include "ssa.hpp"
#include "../utils.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <type_traits>

namespace fish::java::ssa::live {
    using InstIter = InstructionIterator;

    struct CompareInstIter {
        bool operator()(InstIter it1, InstIter it2) const {
            return &*it1 < &*it2;
        }
    };

    using InstSet = std::set<
        InstIter,
        CompareInstIter
    >;

    // Maps an instruction to places where the instruction is live.
    using LifeMap = std::map<
        InstIter,
        std::set<const void*>,
        CompareInstIter
    >;

    // Maps a place to the instructions that are live at that place
    using LiveVarMap = std::map<const void*, InstSet>;

    class LifeMapBuilder {
        using Block = const BasicBlock;
        using Inst = const Instruction;
        using Term = const Terminator;

        public:
        LifeMapBuilder(const Function& func) : m_func(func) {
            decltype(auto) blocks = m_func.blocks();
            if (blocks.begin() == blocks.end()) {
                return;
            }
            while (calculate_once());
        }

        LifeMap& life_map() {
            return m_life_map;
        }

        LiveVarMap& live_var_map() {
            return m_live_var_map;
        }

        private:
        InstSet inputs(InstIter inst) {
            InstSet result;
            inst->visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, Move>) {
                    insert(result, obj.value());
                }
                else if constexpr (std::is_same_v<T, BinaryOperation>) {
                    insert(result, obj.left());
                    insert(result, obj.right());
                }
                else if constexpr (std::is_same_v<T, Comparison>) {
                    insert(result, obj.left());
                    insert(result, obj.right());
                }
                else if constexpr (std::is_same_v<T, FunctionCall>) {
                    for (auto& arg : obj.args()) {
                        insert(result, arg);
                    }
                }
                else if constexpr (std::is_same_v<T, StandardCall>) {
                    for (auto& arg : obj.args()) {
                        insert(result, arg);
                    }
                }
                else if constexpr (std::is_same_v<T, Phi>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Load>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Store>) {
                    insert(result, obj.value());
                }
                else if constexpr (std::is_same_v<T, LoadArgument>) {
                    // Nothing
                }
                else {
                    static_assert(utils::always_false<T>);
                }
            });
            return result;
        }

        InstSet inputs(Term& term) {
            InstSet result;
            term.visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, UnconditionalBranch>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Branch>) {
                    insert(result, obj.cond());
                }
                else if constexpr (std::is_same_v<T, ReturnVoid>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Return>) {
                    insert(result, obj.value());
                }
                else {
                    static_assert(utils::always_false<T>);
                }
            });
            return result;
        }

        InstSet outputs(InstIter inst) {
            InstSet result;
            inst->visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, Move>) {
                    result.insert(inst);
                }
                else if constexpr (std::is_same_v<T, BinaryOperation>) {
                    result.insert(inst);
                }
                else if constexpr (std::is_same_v<T, Comparison>) {
                    result.insert(inst);
                }
                else if constexpr (std::is_same_v<T, FunctionCall>) {
                    if (obj.function().nreturn() > 0) {
                        result.insert(inst);
                    }
                }
                else if constexpr (std::is_same_v<T, StandardCall>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Phi>) {
                    result.insert(inst);
                }
                else if constexpr (std::is_same_v<T, Load>) {
                    result.insert(inst);
                }
                else if constexpr (std::is_same_v<T, Store>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, LoadArgument>) {
                    result.insert(inst);
                }
                else {
                    static_assert(utils::always_false<T>);
                }
            });
            return result;
        }

        InstSet block_live_start(Block& block, Block& pred) {
            InstSet live = m_live[&block];
            for (Inst& inst : block.instructions()) {
                auto ptr = inst.get_if<Phi>();
                if (!ptr) {
                    break;
                }

                const Phi& phi = *ptr;
                for (auto& pair : phi) {
                    if (&pair.block() != &pred) {
                        continue;
                    }

                    auto ptr = pair.value().get_if<InstIter>();
                    if (ptr) {
                        InstIter inst = *ptr;
                        live.insert(inst);
                    }
                }
            }
            return live;
        }

        InstSet block_live_end(Block& block) {
            Term& term = block.terminator();
            InstSet live = inputs(term);
            term.visit([&] (auto& obj) {
                for (Block* succ : obj.successors()) {
                    for (InstIter inst : block_live_start(*succ, block)) {
                        live.insert(inst);
                    }
                }
            });
            return live;
        }

        template <typename T>
        void insert(InstSet& set, T&& value) const {
            if (auto ptr = value.template get_if<InstIter>()) {
                set.insert(*ptr);
            }
        }

        bool calculate_once() {
            decltype(auto) blocks = m_func.blocks();
            auto it = blocks.end();
            auto begin = blocks.begin();
            bool changed = false;

            do {
                Block& block = *--it;
                changed |= calculate(block);
            } while (it != begin);
            return changed;
        }

        bool calculate(Block& block) {
            InstSet live = block_live_end(block);
            for (InstIter inst : live) {
                const void* ptr = &block.terminator();
                m_life_map[inst].insert(ptr);
                m_live_var_map[ptr].insert(inst);
            }

            // Must remove const to get non-const Instruction iterators.
            decltype(auto) instructions = (
                const_cast<BasicBlock&>(block).instructions()
            );

            auto it = instructions.end();
            auto begin = instructions.begin();

            if (it != begin) {
                do {
                    Inst& inst = *--it;
                    InstSet inputs = this->inputs(it);
                    InstSet outputs = this->outputs(it);
                    for (InstIter out : outputs) {
                        live.erase(out);
                    }
                    for (InstIter in : inputs) {
                        live.insert(in);
                    }
                    for (InstIter in : live) {
                        const void* ptr = &inst;
                        m_life_map[in].insert(ptr);
                        m_live_var_map[ptr].insert(in);
                    }
                } while (it != begin);
            }

            InstSet& prev = m_live[&block];
            if (live == prev) {
                return false;
            }
            prev = live;
            return true;
        }

        private:
        const Function& m_func;
        std::map<Block*, InstSet> m_live;
        LiveVarMap m_live_var_map;
        LifeMap m_life_map;
    };

    class InterferenceMap {
        using Map = std::map<
            InstIter,
            InstSet,
            CompareInstIter
        >;

        public:
        using value_type = Map::value_type;
        using iterator = Map::iterator;
        using const_iterator = Map::const_iterator;

        InterferenceMap() = default;

        InterferenceMap(const LifeMap& life) {
            auto it1 = life.begin();
            auto end = life.end();

            for (; it1 != end; ++it1) {
                auto& pair1 = *it1;
                InstIter inst1 = pair1.first;
                auto& lives1 = pair1.second;
                auto& set1 = m_map[inst1];

                auto it2 = it1;
                ++it2;

                for (; it2 != end; ++it2) {
                    auto& pair2 = *it2;
                    InstIter inst2 = pair2.first;
                    auto& lives2 = pair2.second;
                    auto& set2 = m_map[inst2];

                    for (auto life2 : lives2) {
                        if (lives1.count(life2) > 0) {
                            set1.insert(inst2);
                            set2.insert(inst1);
                        }
                    }
                }
            }
        }

        bool empty() const {
            return m_map.empty();
        }

        std::size_t size() const {
            return m_map.size();
        }

        auto begin() {
            return m_map.begin();
        }

        auto begin() const {
            return m_map.begin();
        }

        auto end() {
            return m_map.end();
        }

        auto end() const {
            return m_map.end();
        }

        void add(InstIter inst1, InstIter inst2) {
            m_map[inst1].insert(inst2);
            m_map[inst2].insert(inst1);
        }

        void remove(iterator it) {
            InstIter inst = it->first;
            m_map.erase(it);
            for (auto& pair : m_map) {
                auto& set = pair.second;
                set.erase(inst);
            }
        }

        void remove(InstIter inst) {
            auto it = m_map.find(inst);
            if (it != m_map.end()) {
                remove(it);
            }
        }

        private:
        Map m_map;
    };
}
