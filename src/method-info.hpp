#pragma once
#include "code-info.hpp"
#include "constant-pool.hpp"
#include "method-descriptor.hpp"
#include "stream.hpp"
#include "typedefs.hpp"
#include <stdexcept>
#include <utility>

namespace fish::java::method_info_detail {
    class Parsed {
        public:
        u16 name_index = 0;
        u16 descriptor_index = 0;

        CodeInfo& code() {
            return *m_code;
        }

        Parsed(Stream& stream, const ConstantPool& cpool) :
        name_index((read_start(stream), stream.read_u16())),
        descriptor_index(stream.read_u16())
        {
            // Number of attributes
            const u16 count = stream.read_u16();

            for (u16 i = 0; i < count; ++i) {
                const u16 name_index = stream.read_u16();
                const u32 length = stream.read_u32();
                std::string name = cpool.get<pool::UTF8>(name_index).str;

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

    class MethodInfo {
        public:
        u16 name_index = 0;
        u16 descriptor_index = 0;
        CodeInfo code;

        MethodInfo() = default;

        MethodInfo(Stream& stream, const ConstantPool& cpool) :
        MethodInfo(Parsed(stream, cpool)) {
        }

        MethodDescriptor descriptor(const ConstantPool& cpool) const {
            auto& sig = cpool.get<pool::UTF8>(descriptor_index);
            return MethodDescriptor(sig.str);
        }

        const std::string& name(const ConstantPool& cpool) const {
            auto& name = cpool.get<pool::UTF8>(name_index);
            return name.str;
        }

        pool::NameAndType name_and_type() const {
            return {name_index, descriptor_index};
        }

        private:
        MethodInfo(Parsed parsed) :
        name_index(parsed.name_index),
        descriptor_index(parsed.descriptor_index),
        code(std::move(parsed.code())) {
        }
    };
}

namespace fish::java {
    using method_info_detail::MethodInfo;
}
