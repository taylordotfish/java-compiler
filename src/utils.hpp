#pragma once
#include "stream.hpp"

namespace Fish::Java::Utils {
    void skip_attribute_table(Stream& stream) {
        const u16 count = stream.read_u16();
        for (u16 i = 0; i < count; ++i) {
            stream.read_u16();  // Attribute name index
            const u32 length = stream.read_u32();
            for (u32 j = 0; j < length; ++j) {
                stream.read_u8();  // Info byte
            }
        }
    }
}
