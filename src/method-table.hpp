#pragma once
#include "constant-pool.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace Fish::Java {
    class Code {
        public:
        u16 max_stack = 0;
        u16 max_locals = 0;
        std::vector<u8> code;

        Code() = default;

        Code(Stream& stream) :
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
        std::optional<Code> code;

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
                    if (code) {
                        throw std::runtime_error("Duplicate Code attributes");
                    }
                    code = Code(stream);
                    continue;
                }

                for (u32 j = 0; j < length; ++j) {
                    stream.read_u8();  // Info byte
                }
            }

            if (!code) {
                throw std::runtime_error("Method is missing Code attribute");
            }
        }

        private:
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
        Code code;

        MethodInfo() = default;

        MethodInfo(Stream& stream, const ConstantPool& pool) :
        MethodInfo(Parsed(stream, pool)) {
        }

        private:
        using Parsed = Detail::MethodInfo::Parsed;

        MethodInfo(Parsed parsed) :
        name_index(parsed.name_index),
        descriptor_index(parsed.descriptor_index),
        code(*parsed.code) {
        }
    };

    class MethodTable {
        public:
        MethodTable(Stream& stream, const ConstantPool& pool) {
            const u16 count = stream.read_u16();
            m_entries.reserve(count);
            for (u16 i = 0; i < count; ++i) {
                m_entries.emplace_back(stream, pool);
            }
        }

        private:
        std::vector<MethodInfo> m_entries;
    };
}
