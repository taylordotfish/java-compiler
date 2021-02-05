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
#include <cstdint>
#include <limits>

namespace fish::java::detail {
    template <typename T>
    inline constexpr bool is_twos_complement = (
        static_cast<T>(~static_cast<T>(0)) == static_cast<T>(-1)
    );
}

namespace fish::java {
    using u8 = std::uint_least8_t;
    using u16 = std::uint_least16_t;
    using u32 = std::uint_least32_t;
    using u64 = std::uint_least64_t;

    using s8 = std::int_least8_t;
    using s16 = std::int_least16_t;
    using s32 = std::int_least32_t;
    using s64 = std::int_least64_t;

    static_assert(detail::is_twos_complement<s8>);
    static_assert(detail::is_twos_complement<s16>);
    static_assert(detail::is_twos_complement<s32>);
    static_assert(detail::is_twos_complement<s64>);

    using f32 = float;
    using f64 = double;

    static_assert(std::numeric_limits<f32>::is_iec559);
    static_assert(std::numeric_limits<f64>::is_iec559);
}
