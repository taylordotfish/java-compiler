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
#include "constant-pool.hpp"
#include "method-info.hpp"
#include "stream.hpp"
#include "typedefs.hpp"
#include <stdexcept>
#include <utility>
#include <vector>

namespace fish::java {
    using method_info_detail::MethodInfo;

    class MethodTable {
        using Entries = std::vector<MethodInfo>;

        public:
        using iterator = Entries::iterator;
        using const_iterator = Entries::const_iterator;
        using value_type = Entries::value_type;

        MethodTable(Stream& stream, const ConstantPool& cpool) {
            const u16 count = stream.read_u16();
            m_entries.reserve(count);
            for (u16 i = 0; i < count; ++i) {
                m_entries.emplace_back(stream, cpool);
            }
        }

        const MethodInfo* find(const pool::NameAndType& desc) const {
            for (auto& info : m_entries) {
                if (info.name_index != desc.name_index) continue;
                if (info.descriptor_index != desc.desc_index) continue;
                return &info;
            }
            return nullptr;
        }

        const MethodInfo* main(const ConstantPool& cpool) const {
            for (auto& info : m_entries) {
                auto& name = cpool.get<pool::UTF8>(info.name_index);
                if (name.str == "main") {
                    return &info;
                }
            }
            return nullptr;
        }

        private:
        template <typename This>
        auto begin() const {
            // NOTE: Skipping first entry, which is the class constructor.
            // FIXME: Is this correct?
            return const_cast<This&>(*this).m_entries.begin() + 1;
        }

        public:
        auto begin() {
            return begin<MethodTable>();
        }

        auto begin() const {
            return begin<const MethodTable>();
        }

        auto end() {
            return m_entries.begin();
        }

        auto end() const {
            return m_entries.end();
        }

        private:
        Entries m_entries;
    };
}
