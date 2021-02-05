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
#include <list>
#include <map>
#include <stdexcept>
#include <set>
#include <utility>
#include <vector>

namespace fish::java::ssa {
    class UnlinkedValue {
        public:
        UnlinkedValue(Variable var, Value& value) :
        m_var(var), m_value(&value) {
        }

        Variable var() const {
            return m_var;
        }

        Value& value() {
            assert(m_value);
            return *m_value;
        }

        const Value& value() const {
            return const_cast<UnlinkedValue&>(*this).value();
        }

        private:
        Variable m_var;
        Value* m_value = nullptr;
    };

    using DefMap = std::map<BasicBlock*, std::map<Variable, Value>>;
    using UnlinkedMap = std::map<BasicBlock*, std::list<UnlinkedValue>>;

    class Dominators {
        using Block = const BasicBlock;
        using BlockSet = std::set<Block*>;

        public:
        Dominators(const Function& func) {
            initialize_sets(func);
            while (step_all(func));

            // FIXME: Debug
            if constexpr (false) {
                std::cout << "\n";
                for (auto& block : func.blocks()) {
                    auto& doms = m_dominators.at(&block);
                    std::cout << block.id() << " is dominated by: ";
                    for (auto dom : doms) {
                        std::cout << dom->id() << ", ";
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";

                for (auto& block : func.blocks()) {
                    std::cout << block.id() << " has frontiers: ";
                    for (auto front : frontiers(block)) {
                        std::cout << front->id() << ", ";
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";
            }
        }

        bool dominates(Block& dom, Block& other) {
            return m_dominators.at(&other).count(&dom) > 0;
        }

        bool strictly_dominates(Block& dom, Block& other) {
            return &dom != &other && dominates(dom, other);
        }

        bool frontier(Block& block, Block& front) {
            if (strictly_dominates(block, front)) {
                return false;
            }
            for (Block* pred : front.predecessors()) {
                if (dominates(block, *pred)) {
                    return true;
                }
            }
            return false;
        }

        std::set<Block*> frontiers(Block& block) {
            std::set<Block*> result;
            for (auto& pair : m_dominators) {
                Block* front = pair.first;
                if (frontier(block, *front)) {
                    result.insert(front);
                }
            }
            return result;
        }

        private:
        std::map<Block*, std::set<Block*>> m_dominators;

        void initialize_sets(const Function& func) {
            auto it = func.blocks().begin();
            auto end = func.blocks().end();

            Block& start = *it;
            m_dominators[&start].insert(&start);
            ++it;

            std::set<Block*> all_blocks;
            for (Block& block : func.blocks()) {
                all_blocks.insert(&block);
            }

            for (; it != end; ++it) {
                Block& block = *it;
                m_dominators[&block] = all_blocks;
            }
        }

        bool step_all(const Function& func) {
            auto it = ++func.blocks().begin();
            auto end = func.blocks().end();
            bool changed = false;
            for (; it != end; ++it) {
                Block& block = *it;
                changed |= step_one(block);
            }
            return changed;
        }

        bool step_one(Block& block) {
            auto it = block.predecessors().begin();
            auto end = block.predecessors().end();

            std::set<Block*> next;
            if (it != end) {
                Block* pred = *(it++);
                next = m_dominators.at(pred);
            }

            for (; it != end; ++it) {
                Block* pred = *it;
                intersect(next, m_dominators.at(pred));
            }

            next.insert(&block);
            auto& current = m_dominators.at(&block);
            if (next == current) {
                return false;
            }
            current = std::move(next);
            return true;
        }

        void intersect(BlockSet& dest, BlockSet& source) const {
            auto it = dest.begin();
            auto end = dest.end();
            while (it != end) {
                Block* block = *it;
                if (source.count(block) <= 0) {
                    dest.erase(it++);
                } else {
                    ++it;
                }
            }
        }
    };

    class PhiFixer {
        public:
        PhiFixer(Function& func, DefMap& defs) :
        m_func(func), m_defs(defs), m_doms(func) {
        }

        void fix() {
            auto vars = variables();
            for (auto& var : vars) {
                fix(var);
            }
        }

        void fix(Variable var) {
            std::set<BasicBlock*> work_list;
            std::set<BasicBlock*> has_phi;
            std::vector<InstructionIterator> phis;
            std::set<Instruction*> referenced_instructions;

            for (auto& block : m_func.blocks()) {
                if (defines(block, var)) {
                    work_list.insert(&block);
                }
            }

            std::set<BasicBlock*> done = work_list;
            while (!work_list.empty()) {
                auto it = work_list.begin();
                BasicBlock& block = **it;
                work_list.erase(it);

                for (auto front_ptr : m_doms.frontiers(block)) {
                    BasicBlock& front = const_cast<BasicBlock&>(*front_ptr);
                    if (has_phi.count(&front) > 0) {
                        continue;
                    }

                    auto phi = front.instructions().prepend(Phi());
                    phis.push_back(phi);
                    has_phi.insert(&front);

                    // (var, phi) should be added to m_links, but this
                    // is done later, after the possible removal of phi.
                    //m_links[&front].emplace(var, phi);

                    bool inserted = m_defs[&front].emplace(var, phi).second;
                    if (inserted) {
                        referenced_instructions.insert(&*phi);
                    }

                    if (done.count(&front) > 0) {
                        continue;
                    }
                    work_list.insert(&front);
                    done.insert(&front);
                }
            }

            for (auto& block : m_func.blocks()) {
                if (has_phi.count(&block) > 0) {
                    continue;
                }

                std::set<BasicBlock*> seen;
                BasicBlock* pred = &block;
                seen.insert(pred);
                auto& links = m_links[&block];
                auto& defs = m_defs[&block];

                for (BasicBlock* next : pred->predecessors()) {
                    if (seen.count(next) > 0) {
                        continue;
                    }

                    pred = next;
                    auto& pred_defs = m_defs.at(pred);
                    auto def_iter = pred_defs.find(var);
                    if (def_iter == pred_defs.end()) {
                        seen.insert(pred);
                        continue;
                    }

                    Value& def = def_iter->second;
                    if (auto inst = def.get_if<InstructionIterator>()) {
                        referenced_instructions.insert(&**inst);
                    }

                    links.emplace(var, def);
                    defs.emplace(var, def);
                    break;
                }
            }

            for (auto inst : phis) {
                Phi& phi = inst->get<Phi>();
                auto& block = inst->block();
                bool removed = false;

                for (BasicBlock* pred : block.predecessors()) {
                    auto& defs = m_defs.at(pred);
                    auto it = defs.find(var);
                    if (it != defs.end()) {
                        auto& value = it->second;
                        phi.emplace(*pred, value);
                        continue;
                    }

                    // Remove unnecessary phi. No one should be using it.
                    if (referenced_instructions.count(&*inst) > 0) {
                        throw std::runtime_error("Unnecessary phi has uses");
                    }

                    block.instructions().erase(inst);
                    removed = true;
                    break;
                }

                if (!removed) {
                    m_links[&block].emplace(var, inst);
                }
            }
        }

        const std::map<Variable, Value>& links(BasicBlock& block) {
            return m_links[&block];
        }

        private:
        Function& m_func;
        DefMap& m_defs;
        Dominators m_doms;

        std::map<BasicBlock*, std::map<Variable, Value>> m_links;

        std::set<Variable> variables() const {
            std::set<Variable> result;
            for (auto& [_, map] : m_defs) {
                for (auto& [var, _] : map) {
                    result.insert(var);
                }
            }
            return result;
        }

        bool defines(BasicBlock& block, Variable var) {
            return m_defs.at(&block).count(var) > 0;
        }
    };
}
