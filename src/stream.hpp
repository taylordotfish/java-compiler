#pragma once
#include "typedefs.hpp"
#include <cstddef>
#include <cstring>
#include <istream>

namespace Fish::Java {
    class Stream {
        public:
        Stream(std::istream& stream) : m_stream(stream) {
        }

        u8 read_u8() {
            return get();
        }

        u16 read_u16() {
            return read_integer<u16, 2>();
        }

        u32 read_u32() {
            return read_integer<u32, 4>();
        }

        u32 read_u64() {
            return read_integer<u64, 4>();
        }

        s8 read_s8() {
            return read_u8();
        }

        s16 read_s16() {
            return read_u16();
        }

        s32 read_s32() {
            return read_u32();
        }

        s32 read_s64() {
            return read_u64();
        }

        f32 read_f32() {
            return read_float<f32, u32, 4>();
        }

        f32 read_f64() {
            return read_float<f64, u64, 8>();
        }

        std::size_t pos() const {
            return m_pos;
        }

        private:
        std::istream& m_stream;
        std::size_t m_pos = 0;

        std::istream& stream() {
            return m_stream;
        }

        unsigned char get() {
            unsigned char result = stream().get();
            if (!stream().eof()) {
                ++m_pos;
            }
            return result;
        }

        template <typename Integer, std::size_t nbytes>
        Integer read_integer() {
            Integer result = 0;
            for (std::size_t i = 0; i < nbytes; ++i) {
                result <<= 8;
                result |= get();
            }
            if (stream().eof()) {
                throw std::runtime_error("Unexpected EOF");
            }
            return result;
        }

        template <typename Float, typename Integer, std::size_t nbytes>
        Float read_float() {
            Float result = 0;
            Integer integer = read_integer<Integer, nbytes>();
            auto bytes = reinterpret_cast<char*>(&integer);
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                std::memcpy(&result, bytes, 4);
            #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                std::memcpy(&result, bytes + sizeof(u32) - 4, 4);
            #else
                #error Unsupported endianness
            #endif
            return result;
        }
    };
}
