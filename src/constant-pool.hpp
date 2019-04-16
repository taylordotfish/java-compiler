#pragma once
#include "stream.hpp"
#include "typedefs.hpp"
#include "utils.hpp"
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace fish::java::pool {
    namespace detail {
        struct Base {
            static inline constexpr u16 nslots = 1;
        };
    }

    struct UTF8 : detail::Base {
        std::string str;

        UTF8() = default;

        UTF8(Stream& stream) {
            const u16 length = stream.read_u16();
            str.reserve(length);
            for (u16 i = 0; i < length; ++i) {
                str.push_back(stream.read_u8());
            }
        }
    };

    struct Integer : detail::Base {
        s32 value = 0;

        Integer() = default;

        Integer(Stream& stream) :
        value(stream.read_s32()) {
        }
    };

    struct Float : detail::Base {
        f32 value = 0;

        Float() = default;

        Float(Stream& stream) :
        value(stream.read_f32()) {
        }
    };

    struct Long : detail::Base {
        static constexpr inline u16 nslots = 2;
        s64 value = 0;

        Long() = default;

        Long(Stream& stream) :
        value(stream.read_s64()) {
        }
    };

    struct Double : detail::Base {
        static constexpr inline u16 nslots = 2;
        f64 value = 0;

        Double() = default;

        Double(Stream& stream) :
        value(stream.read_f64()) {
        }
    };

    struct ClassRef : detail::Base {
        u16 index = 0;

        ClassRef() = default;

        ClassRef(Stream& stream) :
        index(stream.read_u16()) {
        }
    };

    struct StringRef : detail::Base {
        u16 index = 0;

        StringRef() = default;

        StringRef(Stream& stream) :
        index(stream.read_u16()) {
        }
    };

    struct BaseMemberRef : detail::Base {
        u16 class_ref_index = 0;
        u16 name_type_index = 0;

        BaseMemberRef() = default;

        BaseMemberRef(Stream& stream) :
        class_ref_index(stream.read_u16()),
        name_type_index(stream.read_u16()) {
        }
    };

    struct FieldRef : BaseMemberRef {
        using BaseMemberRef::BaseMemberRef;
    };

    struct BaseMethodRef : BaseMemberRef {
        using BaseMemberRef::BaseMemberRef;
    };

    struct MethodRef : BaseMethodRef {
        using BaseMethodRef::BaseMethodRef;
    };

    struct InterfaceMethodRef : BaseMethodRef {
        using BaseMethodRef::BaseMethodRef;
    };

    struct NameAndType : detail::Base {
        u16 name_index = 0;
        u16 desc_index = 0;

        NameAndType() = default;

        NameAndType(u16 name_index, u16 desc_index) :
        name_index(name_index), desc_index(desc_index) {
        }

        NameAndType(Stream& stream) :
        name_index(stream.read_u16()),
        desc_index(stream.read_u16()) {
        }

        private:
        auto data() const {
            return std::make_tuple(name_index, desc_index);
        }

        public:
        bool operator==(const NameAndType& other) const {
            return data() == other.data();
        }

        bool operator!=(const NameAndType& other) const {
            return data() != other.data();
        }

        bool operator<(const NameAndType& other) const {
            return data() < other.data();
        }
    };

    struct MethodHandle : detail::Base {
        u8 type_desc = 0;
        u16 index = 0;

        MethodHandle() = default;

        MethodHandle(Stream& stream) :
        type_desc(stream.read_u8()),
        index(stream.read_u16()) {
        }
    };

    struct MethodType : detail::Base {
        u16 index = 0;

        MethodType() = default;

        MethodType(Stream& stream) :
        index(stream.read_u16()) {
        }
    };

    struct Dynamic : detail::Base {
        u32 value = 0;

        Dynamic() = default;

        Dynamic(Stream& stream) :
        value(stream.read_u32()) {
        }
    };

    struct InvokeDynamic : detail::Base {
        u16 value = 0;

        InvokeDynamic() = default;

        InvokeDynamic(Stream& stream) :
        value(stream.read_u16()) {
        }
    };

    struct Module : detail::Base {
        u16 value = 0;

        Module() = default;

        Module(Stream& stream) :
        value(stream.read_u16()) {
        }
    };

    struct Package : detail::Base {
        u16 value = 0;

        Package() = default;

        Package(Stream& stream) :
        value(stream.read_u16()) {
        }
    };
}

namespace fish::java::pool::detail {
    using Variant = std::variant<
        UTF8,
        Integer,
        Float,
        Long,
        Double,
        ClassRef,
        StringRef,
        FieldRef,
        MethodRef,
        InterfaceMethodRef,
        NameAndType,
        MethodHandle,
        MethodType,
        Dynamic,
        InvokeDynamic,
        Module,
        Package
    >;

    class ConstantPool {
        public:
        class Entry : private utils::VariantWrapper<Variant> {
            public:
            using VariantWrapper::VariantWrapper;
            using VariantWrapper::visit;
            using VariantWrapper::get;

            Entry(Stream& stream, u16& nslots) :
            VariantWrapper(make(stream, nslots)) {
            }

            private:
            Variant make(Stream& stream, u16& nslots) const;

            template <typename T>
            T make(Stream& stream, u16& nslots) const {
                nslots = T::nslots;
                return T(stream);
            }
        };

        ConstantPool() = default;

        ConstantPool(Stream& stream) {
            u16 count = stream.read_u16();
            if (count == 0) {
                throw std::runtime_error("Pool count cannot be 0");
            }
            --count;
            for (u16 i = 0; i < count;) {
                u16 nslots = 0;
                m_pool.push_back(Entry(stream, nslots));
                i += nslots;
            }
        }

        Entry& operator[](u16 i) {
            if (i == 0) {
                throw std::runtime_error("Invalid pool index");
            }
            if (--i > m_pool.size()) {
                throw std::runtime_error("Invalid pool index");
            }
            std::optional<Entry>& opt = m_pool[i];
            if (!opt) {
                throw std::runtime_error("Invalid pool index");
            }
            return *opt;
        }

        const Entry& operator[](u16 i) const {
            return const_cast<ConstantPool&>(*this)[i];
        }

        public:
        template <typename T>
        T& get(u16 i) {
            return (*this)[i].visit([] (auto& obj) -> T& {
                constexpr bool error = !std::is_convertible_v<
                    std::decay_t<decltype(obj)>*, T*
                >;
                if constexpr (error) {
                    throw std::runtime_error("Bad pool entry type");
                } else {
                    return obj;
                }
            });
        }

        template <typename T>
        const T& get(u16 i) const {
            return const_cast<ConstantPool&>(*this).get<T>(i);
        }

        private:
        std::vector<std::optional<Entry>> m_pool;
    };
}

namespace fish::java {
    using pool::detail::ConstantPool;
}
