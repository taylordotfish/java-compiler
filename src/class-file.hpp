#pragma once
#include "constant-pool.hpp"
#include "method-table.hpp"
#include "stream.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <stdexcept>

namespace Fish::Java {
    class ClassFile {
        public:
        ConstantPool cpool;
        u16 this_index = 0;
        MethodTable methods;

        ClassFile(Stream& stream) :
        cpool((read_start(stream), stream)),
        this_index((after_cpool(stream), stream.read_u16())),
        methods((after_this_index(stream), stream), cpool)
        {
            Utils::skip_attribute_table(stream);
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

        void after_this_index(Stream& stream) {
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
                Utils::skip_attribute_table(stream);
            }
        }
    };
}
