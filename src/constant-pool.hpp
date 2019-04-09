#pragma once
#include "stream.hpp"
#include "typedefs.hpp"
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace Fish::Java::ConstantPoolEntry {
    namespace Detail {
        struct Base {
            static inline constexpr u16 nslots = 1;
        };

        struct BaseMemberRef : Base {
            u16 class_ref_index = 0;
            u16 name_type_index = 0;

            BaseMemberRef() = default;

            BaseMemberRef(Stream& stream) :
            class_ref_index(stream.read_u16()),
            name_type_index(stream.read_u16()) {
            }
        };
    }

    struct UTF8 : Detail::Base {
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

    struct Integer : Detail::Base {
        s32 value = 0;

        Integer() = default;

        Integer(Stream& stream) :
        value(stream.read_s32()) {
        }
    };

    struct Float : Detail::Base {
        f32 value = 0;

        Float() = default;

        Float(Stream& stream) :
        value(stream.read_f32()) {
        }
    };

    struct Long : Detail::Base {
        static constexpr inline u16 nslots = 2;
        s64 value = 0;

        Long() = default;

        Long(Stream& stream) :
        value(stream.read_s64()) {
        }
    };

    struct Double : Detail::Base {
        static constexpr inline u16 nslots = 2;
        f64 value = 0;

        Double() = default;

        Double(Stream& stream) :
        value(stream.read_f64()) {
        }
    };

    struct ClassRef : Detail::Base {
        u16 index = 0;

        ClassRef() = default;

        ClassRef(Stream& stream) :
        index(stream.read_u16()) {
        }
    };

    struct StringRef : Detail::Base {
        u16 index = 0;

        StringRef() = default;

        StringRef(Stream& stream) :
        index(stream.read_u16()) {
        }
    };

    struct FieldRef : Detail::BaseMemberRef {
        using Detail::BaseMemberRef::BaseMemberRef;
    };

    struct MethodRef : Detail::BaseMemberRef {
        using Detail::BaseMemberRef::BaseMemberRef;
    };

    struct InterfaceMethodRef : Detail::BaseMemberRef {
        using Detail::BaseMemberRef::BaseMemberRef;
    };

    struct NameTypeDesc : Detail::Base {
        u16 name_index = 0;
        u16 type_desc_index = 0;

        NameTypeDesc() = default;

        NameTypeDesc(Stream& stream) :
        name_index(stream.read_u16()),
        type_desc_index(stream.read_u16()) {
        }
    };

    struct MethodHandle : Detail::Base {
        u8 type_desc = 0;
        u16 index = 0;

        MethodHandle() = default;

        MethodHandle(Stream& stream) :
        type_desc(stream.read_u8()),
        index(stream.read_u16()) {
        }
    };

    struct MethodType : Detail::Base {
        u16 index = 0;

        MethodType() = default;

        MethodType(Stream& stream) :
        index(stream.read_u16()) {
        }
    };

    struct Dynamic : Detail::Base {
        u32 value = 0;

        Dynamic() = default;

        Dynamic(Stream& stream) :
        value(stream.read_u32()) {
        }
    };

    struct InvokeDynamic : Detail::Base {
        u16 value = 0;

        InvokeDynamic() = default;

        InvokeDynamic(Stream& stream) :
        value(stream.read_u16()) {
        }
    };

    struct Module : Detail::Base {
        u16 value = 0;

        Module() = default;

        Module(Stream& stream) :
        value(stream.read_u16()) {
        }
    };

    struct Package : Detail::Base {
        u16 value = 0;

        Package() = default;

        Package(Stream& stream) :
        value(stream.read_u16()) {
        }
    };

    namespace Detail {
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
            NameTypeDesc,
            MethodHandle,
            MethodType,
            Dynamic,
            InvokeDynamic,
            Module,
            Package
        >;
    }
}

namespace Fish::Java {
    class ConstantPool {
        public:
        class Entry {
            public:
            template <typename T>
            Entry(T&& obj) : m_variant(std::forward<T>(obj)) {
            }

            Entry(Stream& stream, u16& nslots) :
            m_variant(make(stream, nslots)) {
            }

            template <typename F>
            auto&& visit(F&& func) {
                return std::visit(std::forward<F>(func), m_variant);
            }

            private:
            using Variant = ConstantPoolEntry::Detail::Variant;
            Variant m_variant;
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

        template <typename T>
        T& get(u16 i) {
            return (*this)[i].visit([] (auto& obj) -> T& {
                using U = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<U, T>) {
                    return obj;
                } else {
                    throw std::runtime_error("Bad pool entry type");
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
