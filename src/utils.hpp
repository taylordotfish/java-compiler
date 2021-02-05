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
#include "method-descriptor.hpp"
#include "stream.hpp"
#include "typedefs.hpp"
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace fish::java::utils {
    template <typename T>
    inline constexpr bool always_false = false;

    inline void skip_attribute_table(Stream& stream) {
        const u16 count = stream.read_u16();
        for (u16 i = 0; i < count; ++i) {
            stream.read_u16();  // Attribute name index
            const u32 length = stream.read_u32();
            for (u32 j = 0; j < length; ++j) {
                stream.read_u8();  // Info byte
            }
        }
    }

    inline void check_print_method_descriptor(
            const MethodDescriptor& mdesc,
            const std::string& name) {

        if (mdesc.nargs() > 1) {
            std::ostringstream msg;
            msg << "Too many arguments to " << name << ": ";
            msg << mdesc.nargs();
            throw std::runtime_error(msg.str());
        }

        if (mdesc.rtype() != "V") {
            std::ostringstream msg;
            msg << "Invalid return type for " << name << ": ";
            msg << mdesc.rtype();
            throw std::runtime_error(msg.str());
        }

        if (mdesc.nargs() == 0) {
        } else if (mdesc.arg(0) == "C") {
        } else if (mdesc.arg(0) == "I") {
        } else {
            std::ostringstream msg;
            msg << "Invalid argument type for " << name << ": ";
            msg << mdesc.arg(0);
            throw std::runtime_error(msg.str());
        }
    }

    template <typename Variant>
    class VariantWrapper {
        public:
        VariantWrapper() = default;

        template <typename T>
        explicit VariantWrapper(T&& obj) : m_variant(std::forward<T>(obj)) {
        }

        template <typename Fn>
        decltype(auto) visit(Fn&& func) {
            using std::visit;
            return visit(std::forward<Fn>(func), m_variant);
        }

        template <typename Fn>
        decltype(auto) visit(Fn&& func) const {
            using std::visit;
            return visit(std::forward<Fn>(func), m_variant);
        }

        template <typename T>
        decltype(auto) get() {
            using std::get;
            return get<T>(m_variant);
        }

        template <typename T>
        decltype(auto) get() const {
            using std::get;
            return get<T>(m_variant);
        }

        template <typename T>
        decltype(auto) get_if() {
            using std::get_if;
            return get_if<T>(&m_variant);
        }

        template <typename T>
        decltype(auto) get_if() const {
            using std::get_if;
            return get_if<T>(&m_variant);
        }

        private:
        Variant m_variant;
    };

    inline std::string indent(const std::string& str, std::size_t amount) {
        std::string indent(amount, ' ');
        std::string result;
        result.reserve(str.length());

        char prev = '\n';
        for (char chr : str) {
            if (chr != '\n' && prev == '\n') {
                result += indent;
            }
            result += chr;
            prev = chr;
        }
        return result;
    }

    template <typename T>
    std::string str(T&& obj) {
        std::ostringstream stream;
        stream << obj;
        return stream.str();
    }

    template <typename T>
    class Ref {
        public:
        Ref(T& obj) : m_ptr(&obj) {
        }

        Ref(Ref& other) : m_ptr(other.m_ptr) {
        }

        Ref(const Ref&) = delete;

        T& get() const {
            return *m_ptr;
        }

        operator T&() const {
            return get();
        }

        bool operator==(const Ref& other) const {
            return m_ptr == other.m_ptr;
        }

        bool operator!=(const Ref& other) const {
            return m_ptr != other.m_ptr;
        }

        bool operator<(const Ref& other) const {
            return m_ptr < other.m_ptr;
        }

        bool operator<=(const Ref& other) const {
            return m_ptr <= other.m_ptr;
        }

        bool operator>(const Ref& other) const {
            return m_ptr > other.m_ptr;
        }

        bool operator>=(const Ref& other) const {
            return m_ptr >= other.m_ptr;
        }

        private:
        T* m_ptr = nullptr;
    };
}

template <typename T>
struct std::hash<fish::java::utils::Ref<T>> {
    auto operator()(const fish::java::utils::Ref<T>& self) {
        return std::hash(&self.get())()();
    }
};
