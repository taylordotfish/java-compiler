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

namespace fish::java {
    enum class Opcode {
        iconst_m1 = 0x2,
        iconst_0 = 0x3,
        iconst_1 = 0x4,
        iconst_2 = 0x5,
        iconst_3 = 0x6,
        iconst_4 = 0x7,
        iconst_5 = 0x8,

        iload = 0x15,
        iload_0 = 0x1a,
        iload_1 = 0x1b,
        iload_2 = 0x1c,
        iload_3 = 0x1d,

        istore = 0x36,
        istore_0 = 0x3b,
        istore_1 = 0x3c,
        istore_2 = 0x3d,
        istore_3 = 0x3e,

        iinc = 0x84,
        iadd = 0x60,
        isub = 0x64,
        imul = 0x68,
        ishl = 0x78,
        ishr = 0x7a,

        if_icmpeq = 0x9f,
        if_icmpne = 0xa0,
        if_icmpgt = 0xa3,
        if_icmpge = 0xa2,
        if_icmplt = 0xa1,
        if_icmple = 0xa4,

        ifeq = 0x99,
        ifne = 0x9a,
        ifgt = 0x9d,
        ifge = 0x9c,
        iflt = 0x9b,
        ifle = 0x9e,

        Goto = 0xa7,
        bipush = 0x10,
        sipush = 0x11,
        invokestatic = 0xb8,
        invokevirtual = 0xb6,
        Return = 0xb1,
        ireturn = 0xac,
        getstatic = 0xb2,
        pop = 0x57,
    };
}
