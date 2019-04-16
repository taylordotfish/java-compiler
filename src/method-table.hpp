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
