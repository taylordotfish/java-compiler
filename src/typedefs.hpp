#pragma once
#include <cstdint>
#include <limits>

namespace Fish::Java::Detail {
    template <typename T>
    inline constexpr bool is_twos_complement = (
        static_cast<T>(~static_cast<T>(0)) == static_cast<T>(-1)
    );
}

namespace Fish::Java {
    using u8 = std::uint_least8_t;
    using u16 = std::uint_least16_t;
    using u32 = std::uint_least32_t;
    using u64 = std::uint_least64_t;

    using s8 = std::int_least8_t;
    using s16 = std::int_least16_t;
    using s32 = std::int_least32_t;
    using s64 = std::int_least64_t;

    static_assert(Detail::is_twos_complement<s8>);
    static_assert(Detail::is_twos_complement<s16>);
    static_assert(Detail::is_twos_complement<s32>);
    static_assert(Detail::is_twos_complement<s64>);

    using f32 = float;
    using f64 = double;

    static_assert(std::numeric_limits<f32>::is_iec559);
    static_assert(std::numeric_limits<f64>::is_iec559);
}
