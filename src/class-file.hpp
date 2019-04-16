#pragma once
#include "constant-pool.hpp"
#include "method-table.hpp"
#include "stream.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <stdexcept>

namespace fish::java {
    class ClassFile {
        public:
        ConstantPool cpool;
        u16 self_index = 0;
        MethodTable methods;

        ClassFile(Stream& stream) :
        cpool((read_start(stream), stream)),
        self_index((after_cpool(stream), stream.read_u16())),
        methods((after_self_index(stream), stream), cpool)
        {
            utils::skip_attribute_table(stream);
        }

        ClassFile(std::istream& stream) :
        ClassFile(make(stream)) {
        }

        private:
        static ClassFile make(std::istream& is) {
            Stream stream(is);
            return ClassFile(stream);
        }

        void read_start(Stream& stream) {
            if (stream.read_u32() != 0xCAFEBABE) {
                throw std::runtime_error("Bad magic number");
            }
            stream.read_u16();  // Minor version
            stream.read_u16();  // Major version
        }

        void after_cpool(Stream& stream) {
            stream.read_u16();  // Access flags
        }

        void after_self_index(Stream& stream) {
            stream.read_u16();  // Super class index
            read_interface_table(stream);
            read_field_table(stream);
        }

        void read_interface_table(Stream& stream) {
            const u16 count = stream.read_u16();
            for (u16 i = 0; i < count; ++i) {
                stream.read_u16();  // Index
            }
        }

        void read_field_table(Stream& stream) {
            const u16 count = stream.read_u16();
            for (u16 i = 0; i < count; ++i) {
                stream.read_u16();  // Access flags
                stream.read_u16();  // Name index
                stream.read_u16();  // Descriptor index
                utils::skip_attribute_table(stream);
            }
        }
    };
}
