#include "constant-pool.hpp"

namespace Fish::Java {
    auto ConstantPool::Entry::make(Stream& stream, u16& nslots)
    const -> Variant {
        namespace CPE = ConstantPoolEntry;
        const u8 tag = stream.read_u8();
        switch (tag) {
            case 1: {
                return make<CPE::UTF8>(stream, nslots);
            }

            case 3: {
                return make<CPE::Integer>(stream, nslots);
            }

            case 4: {
                return make<CPE::Float>(stream, nslots);
            }

            case 5: {
                return make<CPE::Long>(stream, nslots);
            }

            case 6: {
                return make<CPE::Double>(stream, nslots);
            }

            case 7: {
                return make<CPE::ClassRef>(stream, nslots);
            }

            case 8: {
                return make<CPE::StringRef>(stream, nslots);
            }

            case 9: {
                return make<CPE::FieldRef>(stream, nslots);
            }

            case 10: {
                return make<CPE::MethodRef>(stream, nslots);
            }

            case 11: {
                return make<CPE::InterfaceMethodRef>(stream, nslots);
            }

            case 12: {
                return make<CPE::NameTypeDesc>(stream, nslots);
            }

            case 15: {
                return make<CPE::MethodHandle>(stream, nslots);
            }

            case 16: {
                return make<CPE::MethodType>(stream, nslots);
            }

            case 17: {
                return make<CPE::Dynamic>(stream, nslots);
            }

            case 18: {
                return make<CPE::InvokeDynamic>(stream, nslots);
            }

            case 19: {
                return make<CPE::Module>(stream, nslots);
            }

            case 20: {
                return make<CPE::Package>(stream, nslots);
            }

            default: {
                throw std::runtime_error("Unknown constant pool entry tag");
            }
        }
    }
}
