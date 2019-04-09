#pragma once
#include "constant-pool.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Fish::Java {
    class CodeInfo {
        public:
        u16 max_stack = 0;
        u16 max_locals = 0;
        std::vector<u8> code;

        CodeInfo() = default;

        CodeInfo(Stream& stream) :
        max_stack(stream.read_u16()),
        max_locals(stream.read_u16())
        {
            read_code(stream);
            read_exc_table(stream);
            Utils::skip_attribute_table(stream);
        }

        private:
        void read_code(Stream& stream) {
            const u32 length = stream.read_u32();
            code.reserve(length);
            for (u32 i = 0; i < length; ++i) {
                code.push_back(stream.read_u8());
            }
        }

        void read_exc_table(Stream& stream) {
            const u16 count = stream.read_u16();
            for (u16 i = 0; i < count; ++i) {
                for (u16 j = 0; j < 4; ++j) {
                    stream.read_u16();
                }
            }
        }
    };
}

namespace Fish::Java::Detail::MethodInfo {
    class Parsed {
        public:
        u16 name_index = 0;
        u16 descriptor_index = 0;

        CodeInfo& code() {
            return *m_code;
        }

        Parsed(Stream& stream, const ConstantPool& pool) :
        name_index((read_start(stream), stream.read_u16())),
        descriptor_index(stream.read_u16())
        {
            namespace CPE = ConstantPoolEntry;
            const u16 count = stream.read_u16();

            for (u16 i = 0; i < count; ++i) {
                const u16 name_index = stream.read_u16();
                const u32 length = stream.read_u32();
                std::string name = pool.get<CPE::UTF8>(name_index).str;

                if (name == "Code") {
                    if (m_code) {
                        throw std::runtime_error("Duplicate Code attribute");
                    }
                    m_code = CodeInfo(stream);
                    continue;
                }

                // Skip rest of attribute
                for (u32 j = 0; j < length; ++j) {
                    stream.read_u8();  // Info byte
                }
            }

            // Ensure required attributes were found
            if (!m_code) {
                throw std::runtime_error("Method is missing Code attribute");
            }
        }

        private:
        std::optional<CodeInfo> m_code;

        void read_start(Stream& stream) {
            stream.read_u16();  // Access flags
        }
    };
}

namespace Fish::Java {
    class MethodInfo {
        public:
        u16 name_index = 0;
        u16 descriptor_index = 0;
        CodeInfo code;

        MethodInfo() = default;

        MethodInfo(Stream& stream, const ConstantPool& pool) :
        MethodInfo(Parsed(stream, pool)) {
        }

        private:
        using Parsed = Detail::MethodInfo::Parsed;

        MethodInfo(Parsed parsed) :
        name_index(parsed.name_index),
        descriptor_index(parsed.descriptor_index),
        code(std::move(parsed.code())) {
        }
    };

    class MethodTable {
        public:
        using NameTypeDesc = ConstantPoolEntry::NameTypeDesc;

        MethodTable(Stream& stream, const ConstantPool& pool) {
            const u16 count = stream.read_u16();
            m_entries.reserve(count);
            for (u16 i = 0; i < count; ++i) {
                m_entries.emplace_back(stream, pool);
            }
        }

        const MethodInfo* find(const NameTypeDesc& desc) const {
            for (auto& info : m_entries) {
                if (info.name_index != desc.name_index) continue;
                if (info.descriptor_index != desc.type_desc_index) continue;
                return &info;
            }
            return nullptr;
        }

        const MethodInfo* main(const ConstantPool& pool) const {
            for (auto& info : m_entries) {
                auto& name = pool.get<ConstantPoolEntry::UTF8>(
                    info.name_index
                );
                if (name.str == "main") {
                    return &info;
                }
            }
            return nullptr;
        }

        private:
        std::vector<MethodInfo> m_entries;
    };
}
