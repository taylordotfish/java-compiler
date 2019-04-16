#include "constant-pool.hpp"

namespace fish::java::pool::detail {
    Variant ConstantPool::Entry::make(Stream& stream, u16& nslots) const {
        const u8 tag = stream.read_u8();
        switch (tag) {
            case 1: {
                return make<UTF8>(stream, nslots);
            }

            case 3: {
                return make<Integer>(stream, nslots);
            }

            case 4: {
                return make<Float>(stream, nslots);
            }

            case 5: {
                return make<Long>(stream, nslots);
            }

            case 6: {
                return make<Double>(stream, nslots);
            }

            case 7: {
                return make<ClassRef>(stream, nslots);
            }

            case 8: {
                return make<StringRef>(stream, nslots);
            }

            case 9: {
                return make<FieldRef>(stream, nslots);
            }

            case 10: {
                return make<MethodRef>(stream, nslots);
            }

            case 11: {
                return make<InterfaceMethodRef>(stream, nslots);
            }

            case 12: {
                return make<NameAndType>(stream, nslots);
            }

            case 15: {
                return make<MethodHandle>(stream, nslots);
            }

            case 16: {
                return make<MethodType>(stream, nslots);
            }

            case 17: {
                return make<Dynamic>(stream, nslots);
            }

            case 18: {
                return make<InvokeDynamic>(stream, nslots);
            }

            case 19: {
                return make<Module>(stream, nslots);
            }

            case 20: {
                return make<Package>(stream, nslots);
            }

            default: {
                throw std::runtime_error("Unknown constant pool entry tag");
            }
        }
    }
}
