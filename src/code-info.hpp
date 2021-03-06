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

#pragma once
#include "stream.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <vector>

namespace fish::java {
    using CodeSeq = std::vector<u8>;

    class CodeInfo {
        public:
        u16 max_stack = 0;
        u16 max_locals = 0;
        CodeSeq code;

        CodeInfo() = default;

        CodeInfo(Stream& stream) :
        max_stack(stream.read_u16()),
        max_locals(stream.read_u16())
        {
            read_code(stream);
            read_exc_table(stream);
            utils::skip_attribute_table(stream);
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

