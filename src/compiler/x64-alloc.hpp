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
#include "ssa-live.hpp"
#include "x64.hpp"
#include "../utils.hpp"
#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>

namespace fish::java::x64::reg_alloc_detail {
    using ssa::live::InstIter;
    using ssa::live::InstSet;
    using ssa::live::CompareInstIter;
    using ssa::live::LifeMap;
    using ssa::live::LiveVarMap;
    using ssa::live::LifeMapBuilder;
    using ssa::live::InterferenceMap;

    static inline std::array<Register, 13> registers = {
        Register::rax,
        Register::rbx,
        Register::rdx,
        Register::rsi,
        Register::rdi,
        Register::r8,
        Register::r9,
        Register::r10,
        Register::r11,
        Register::r12,
        Register::r13,
        Register::r14,
        Register::r15,
    };

    using RegMap = std::map<InstIter, Register, CompareInstIter>;

    class RegisterAllocator {
        public:
        RegisterAllocator(ssa::Function& func) : m_func(func) {
        }

        void allocate() {
            while (!step());
        }

        RegMap& regs() {
            return m_regs;
        }

        LiveVarMap& live_var_map() {
            return m_live_var_map;
        }

        private:
        ssa::Function& m_func;
        RegMap m_regs;
        LiveVarMap m_live_var_map;

        bool step();
    };

    inline bool RegisterAllocator::step() {
        m_regs.clear();
        LifeMapBuilder builder(m_func);
        InterferenceMap imap(builder.life_map());

        std::list<std::pair<InstIter, InstSet>> removed;
        bool any = false;
        do {
            any = false;
            auto it = imap.begin();
            auto end = imap.end();
            for (; it != end; ++it) {
                if (it->second.size() < registers.size()) {
                    removed.emplace_front(it->first, std::move(it->second));
                    imap.remove(it);
                    any = true;
                    break;
                }
            }
        } while (any);

        if (!imap.empty()) {
            auto max = std::max_element(
                imap.begin(), imap.end(), [] (auto& pair1, auto& pair2) {
                    return pair1.second.size() < pair2.second.size();
                }
            );

            InstIter inst = max->first;
            InstIter next = inst;
            ++next;

            ssa::BasicBlock& block = inst->block();
            std::size_t slot = m_func.stack_slots()++;
            auto store = block.instructions().insert(
                next, ssa::Store(slot, inst)
            );

            for (auto& block : m_func.blocks()) {
                decltype(auto) instructions = block.instructions();
                auto it = instructions.begin();
                auto end = instructions.end();
                for (; it != end; ++it) {
                    if (&*it == &*store) continue;
                    for (auto value : it->inputs()) {
                        auto ptr = value->get_if<InstIter>();
                        if (!ptr) continue;
                        if (&**ptr != &*inst) continue;
                        auto load = instructions.insert(it, ssa::Load(slot));
                        *value = ssa::Value(load);
                    }
                }

                for (auto value : block.terminator().inputs()) {
                    auto ptr = value->get_if<InstIter>();
                    if (!ptr) continue;
                    if (&**ptr != &*inst) continue;
                    auto load = instructions.append(ssa::Load(slot));
                    *value = ssa::Value(load);
                }
            }
            return false;
        }

        for (auto& [inst, neighbors] : removed) {
            bool conflict = false;
            for (Register reg : registers) {
                conflict = false;
                for (InstIter neighbor : neighbors) {
                    auto alloc = m_regs.find(neighbor);
                    if (alloc == m_regs.end()) {
                        continue;
                    }
                    if (alloc->second == reg) {
                        conflict = true;
                        break;
                    }
                }
                if (!conflict) {
                    m_regs.emplace(inst, reg);
                    break;
                }
            }
            if (conflict) {
                throw std::runtime_error("Can't allocate!");
            }
        }

        m_live_var_map = std::move(builder.live_var_map());
        return true;
    }
}

namespace fish::java::x64 {
    using reg_alloc_detail::RegMap;
    using reg_alloc_detail::LiveVarMap;
    using reg_alloc_detail::RegisterAllocator;
}
