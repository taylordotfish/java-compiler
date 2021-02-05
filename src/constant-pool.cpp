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
