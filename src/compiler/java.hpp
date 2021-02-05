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
#include "../typedefs.hpp"
#include "../utils.hpp"
#include <bitset>
#include <cassert>
#include <cstddef>
#include <list>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace fish::java::java {
    class Instruction;
    class Function;

    using InstructionList = std::list<Instruction>;
    using InstructionIterator = InstructionList::iterator;
    using ConstInstructionIterator = InstructionList::const_iterator;

    class Constant {
        public:
        Constant(u64 value) : m_value(value) {
        }

        u64& value() {
            return m_value;
        }

        const u64& value() const {
            return m_value;
        }

        private:
        u64 m_value = 0;

        friend std::ostream&
        operator<<(std::ostream& stream, const Constant& self) {
            stream << self.value();
            return stream;
        }
    };

    class Variable {
        public:
        enum class Location {
            stack,
            locals,
        };

        static constexpr Location stack = Location::stack;
        static constexpr Location locals = Location::locals;

        Variable(Location location, u64 index) :
        m_location(location), m_index(index) {
        }

        Location location() const {
            return m_location;
        }

        u64 index() const {
            return m_index;
        }

        bool operator<(const Variable& other) const {
            if (location() < other.location()) return true;
            if (location() > other.location()) return false;
            return index() < other.index();
        }

        private:
        Location m_location = {};
        u64 m_index = 0;

        friend std::ostream&
        operator<<(std::ostream& stream, Location location) {
            switch (location) {
                case Location::stack: {
                    stream << "stack";
                    break;
                }
                case Location::locals: {
                    stream << "local";
                    break;
                }
                default: {
                    stream << "?";
                    break;
                }
            }
            return stream;
        }

        friend std::ostream&
        operator<<(std::ostream& stream, const Variable& self) {
            stream << self.location() << "_" << self.index();
            return stream;
        }
    };

    namespace variants {
        using Value = std::variant<
            Constant,
            Variable
        >;
    }

    class Value : private utils::VariantWrapper<variants::Value> {
        public:
        using VariantWrapper::VariantWrapper;
        using VariantWrapper::visit;
        using VariantWrapper::get;
        using VariantWrapper::get_if;

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const Value& self) {
            self.visit([&] (auto& obj) -> decltype(auto) {
                stream << obj;
            });
            return stream;
        }
    };

    enum class ArithmeticOperator {
        add,
        sub,
        mul,
        shl,
        shr,
    };

    std::ostream&
    operator<<(std::ostream& stream, ArithmeticOperator op) {
        switch (op) {
            case ArithmeticOperator::add: {
                stream << "+";
                break;
            }
            case ArithmeticOperator::sub: {
                stream << "-";
                break;
            }
            case ArithmeticOperator::mul: {
                stream << "*";
                break;
            }
            case ArithmeticOperator::shl: {
                stream << "<<";
                break;
            }
            case ArithmeticOperator::shr: {
                stream << ">>";
                break;
            }
            default: {
                stream << "?";
                break;
            }
        }
        return stream;
    }

    enum class ComparisonOperator {
        eq,
        ne,
        lt,
        le,
        gt,
        ge,
    };

    std::ostream&
    operator<<(std::ostream& stream, ComparisonOperator op) {
        switch (op) {
            case ComparisonOperator::eq: {
                stream << "==";
                break;
            }
            case ComparisonOperator::ne: {
                stream << "!=";
                break;
            }
            case ComparisonOperator::lt: {
                stream << "<";
                break;
            }
            case ComparisonOperator::le: {
                stream << "<=";
                break;
            }
            case ComparisonOperator::gt: {
                stream << ">";
                break;
            }
            case ComparisonOperator::ge: {
                stream << ">=";
                break;
            }
            default: {
                stream << "?";
                break;
            }
        }
        return stream;
    }

    class Move {
        public:
        template <typename Source>
        Move(Source&& source, Variable dest) :
        m_source(std::forward<Source>(source)),
        m_dest(dest) {
        }

        auto& source() {
            return m_source;
        }

        auto& source() const {
            return m_source;
        }

        auto& dest() {
            return m_dest;
        }

        auto dest() const {
            return m_dest;
        }

        private:
        Value m_source;
        Variable m_dest;

        friend std::ostream&
        operator<<(std::ostream& stream, const Move& self) {
            stream << self.dest() << " = " << self.source();
            return stream;
        }
    };

    class BinaryOperation {
        public:
        using Op = ArithmeticOperator;

        template <typename Left, typename Right>
        BinaryOperation(Op op, Left&& left, Right&& right, Variable dest) :
        m_op(op),
        m_left(std::forward<Left>(left)),
        m_right(std::forward<Right>(right)),
        m_dest(dest) {
        }

        auto& op() {
            return m_op;
        }

        auto op() const {
            return m_op;
        }

        auto& left() {
            return m_left;
        }

        auto& left() const {
            return m_left;
        }

        auto& right() {
            return m_right;
        }

        auto& right() const {
            return m_right;
        }

        auto& dest() {
            return m_dest;
        }

        auto dest() const {
            return m_dest;
        }

        private:
        Op m_op = {};
        Value m_left;
        Value m_right;
        Variable m_dest;

        friend std::ostream&
        operator<<(std::ostream& stream, const BinaryOperation& self) {
            stream << self.dest() << " = ";
            stream << self.left() << " " << self.op() << " " << self.right();
            return stream;
        }
    };

    class BranchInst {
        protected:
        BranchInst() = default;

        BranchInst(InstructionIterator target) : m_target(target) {
        }

        auto& target() {
            assert(m_target);
            return *m_target;
        }

        auto& target() const {
            return const_cast<BranchInst&>(*this).target();
        }

        auto& target(std::nullopt_t) {
            return m_target;
        }

        private:
        std::optional<InstructionIterator> m_target;
    };

    class Branch : private BranchInst {
        public:
        using Op = ComparisonOperator;
        using InstIter = InstructionIterator;

        template <typename Left, typename Right>
        Branch(Op op, Left&& left, Right&& right) :
        m_op(op),
        m_left(std::forward<Left>(left)),
        m_right(std::forward<Right>(right)) {
        }

        template <typename Left, typename Right>
        Branch(Op op, Left&& left, Right&& right, InstIter target) :
        BranchInst(target),
        m_op(op),
        m_left(std::forward<Left>(left)),
        m_right(std::forward<Right>(right)) {
        }

        using BranchInst::target;

        auto& op() {
            return m_op;
        }

        auto op() const {
            return m_op;
        }

        auto& left() {
            return m_left;
        }

        auto& left() const {
            return m_left;
        }

        auto& right() {
            return m_right;
        }

        auto& right() const {
            return m_right;
        }

        private:
        Op m_op = {};
        Value m_left;
        Value m_right;

        friend std::ostream&
        operator<<(std::ostream& stream, const Branch& self) {
            stream << self.left() << " " << self.op() << " " << self.right();
            stream << " => goto " << static_cast<const void*>(&*self.target());
            return stream;
        }
    };

    class UnconditionalBranch : private BranchInst {
        public:
        using BranchInst::BranchInst;
        using BranchInst::target;

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const UnconditionalBranch& self) {
            stream << "goto " << static_cast<const void*>(&*self.target());
            return stream;
        }
    };

    class Return {
        public:
        template <typename T>
        Return(T&& value) : m_value(std::forward<T>(value)) {
        }

        auto& value() {
            return m_value;
        }

        auto& value() const {
            return m_value;
        }

        private:
        Value m_value;

        friend std::ostream&
        operator<<(std::ostream& stream, const Return& self) {
            stream << "return " << self.value();
            return stream;
        }
    };

    class ReturnVoid {
        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const ReturnVoid&) {
            stream << "return";
            return stream;
        }
    };

    class BaseFunctionCall {
        protected:
        using ArgList = std::list<Value>;

        template <typename... Args>
        BaseFunctionCall(Args&&... args) :
        m_args({Value(std::forward<Args>(args))...}) {
        }

        ArgList& args() {
            return m_args;
        }

        const ArgList& args() const {
            return m_args;
        }

        private:
        ArgList m_args;
    };

    class FunctionCall : private BaseFunctionCall {
        public:
        template <typename... Args>
        FunctionCall(Function& function, Args&&... args) :
        BaseFunctionCall(std::forward<Args>(args)...),
        m_function(&function) {
        }

        Function& function() {
            assert(m_function);
            return *m_function;
        }

        const Function& function() const {
            return const_cast<FunctionCall&>(*this).function();
        }

        auto& dest() {
            return m_dest;
        }

        auto& dest() const {
            return m_dest;
        }

        using BaseFunctionCall::args;

        private:
        Function* m_function = nullptr;
        std::optional<Variable> m_dest;

        friend std::ostream&
        operator<<(std::ostream& stream, const FunctionCall& self);
    };

    namespace standard_call_detail {
        enum class Kind {
            print_int,
            print_char,
            println_int,
            println_char,
            println_void,
        };

        inline std::size_t nargs(Kind kind) {
            switch (kind) {
                case Kind::println_void: {
                    return 0;
                }

                case Kind::print_int:
                case Kind::print_char:
                case Kind::println_int:
                case Kind::println_char: {
                    return 1;
                }

                default: {
                    throw std::runtime_error("Invalid StandardCall::Kind");
                    return -1;
                }
            }
        }

        inline std::ostream&
        operator<<(std::ostream& stream, Kind kind) {
            switch (kind) {
                case Kind::print_int:
                case Kind::print_char: {
                    stream << "print";
                    break;
                }

                case Kind::println_int:
                case Kind::println_char:
                case Kind::println_void: {
                    stream << "println";
                    break;
                }

                default: {
                    stream << "?";
                    break;
                }
            }
            return stream;
        }
    }

    class StandardCall : private BaseFunctionCall {
        public:
        using Kind = standard_call_detail::Kind;
        using BaseFunctionCall::ArgList;

        template <typename... Args>
        StandardCall(Kind kind, Args&&... args) :
        BaseFunctionCall(std::forward<Args>(args)...),
        m_kind(kind) {
        }

        Kind& kind() {
            return m_kind;
        }

        Kind kind() const {
            return m_kind;
        }

        using BaseFunctionCall::args;

        private:
        Kind m_kind = {};

        friend std::ostream&
        operator<<(std::ostream& stream, const StandardCall& self) {
            stream << "call " << self.kind() << "(";
            std::size_t i = 0;
            for (auto& arg : self.args()) {
                if (i > 0) {
                    stream << ", ";
                }
                stream << arg;
                ++i;
            }
            stream << ")";
            return stream;
        }
    };

    namespace variants {
        using Instruction = std::variant<
            Move,
            BinaryOperation,
            Branch,
            UnconditionalBranch,
            Return,
            ReturnVoid,
            FunctionCall,
            StandardCall
        >;
    }

    class Instruction : private utils::VariantWrapper<variants::Instruction> {
        public:
        class Flags {
            using Bits = std::bitset<1>;

            public:
            using Ref = Bits::reference;

            Ref target() {
                return m_bits[0];
            }

            bool target() const {
                return m_bits[0];
            }

            private:
            Bits m_bits;

            friend std::ostream&
            operator<<(std::ostream& stream, const Flags& self) {
                if (self.target()) {
                    stream << "T";
                } else {
                    stream << "-";
                }
                return stream;
            }
        };

        using VariantWrapper::VariantWrapper;
        using VariantWrapper::visit;
        using VariantWrapper::get;
        using VariantWrapper::get_if;

        Flags& flags() {
            return m_flags;
        }

        const Flags& flags() const {
            return m_flags;
        }

        private:
        Flags m_flags = {};

        friend std::ostream&
        operator<<(std::ostream& stream, const Instruction& self) {
            stream << self.flags() << " ";
            stream << static_cast<const void*>(&self) << "  ";
            self.visit([&] (auto& obj) -> decltype(auto) {
                stream << obj;
            });
            return stream;
        }
    };

    class InstructionSequence {
        public:
        using iterator = InstructionIterator;
        using const_iterator = ConstInstructionIterator;

        auto begin() {
            return m_instructions.begin();
        }

        auto begin() const {
            return m_instructions.begin();
        }

        auto end() {
            return m_instructions.end();
        }

        auto end() const {
            return m_instructions.end();
        }

        template <typename T>
        auto append(T&& instruction) {
            return m_instructions.emplace(
                m_instructions.end(), std::forward<T>(instruction)
            );
        }

        private:
        InstructionList m_instructions;
    };

    class Function {
        public:
        Function(std::size_t nargs, std::size_t nreturn, std::string name) :
        m_nargs(nargs), m_nreturn(nreturn), m_name(std::move(name)) {
        }

        std::size_t nargs() const {
            return m_nargs;
        }

        std::size_t nreturn() const {
            return m_nreturn;
        }

        std::string& name() {
            return m_name;
        }

        const std::string& name() const {
            return m_name;
        }

        auto& instructions() {
            return m_instructions;
        }

        auto& instructions() const {
            return m_instructions;
        }

        private:
        InstructionSequence m_instructions;
        std::size_t m_nargs = 0;
        std::size_t m_nreturn = 0;
        std::string m_name;

        friend std::ostream&
        operator<<(std::ostream& stream, const Function& self) {
            stream << "function " << self.name() << " ";
            stream << "(" << self.nargs() << ") {\n";
            for (auto& instruction : self.instructions()) {
                stream << utils::indent(utils::str(instruction), 4) << "\n";
            }
            stream << "}";
            return stream;
        }
    };

    class FunctionSet {
        using List = std::list<Function>;

        public:
        using iterator = List::iterator;

        auto begin() {
            return m_functions.begin();
        }

        auto begin() const {
            return m_functions.begin();
        }

        auto end() {
            return m_functions.end();
        }

        auto end() const {
            return m_functions.end();
        }

        auto add(const Function& function) {
            return emplace(function);
        }

        auto add(Function&& function) {
            return emplace(std::move(function));
        }

        private:
        List m_functions;

        template <typename... Args>
        iterator emplace(Args&&... args) {
            return m_functions.emplace(
                m_functions.end(), std::forward<Args>(args)...
            );
        }
    };

    class Program {
        public:
        Program() = default;

        auto& functions() {
            return m_functions;
        }

        auto& functions() const {
            return m_functions;
        }

        private:
        FunctionSet m_functions;

        friend std::ostream&
        operator<<(std::ostream& stream, const Program& self) {
            std::size_t i = 0;
            for (auto& function : self.functions()) {
                if (i > 0) {
                    stream << "\n\n";
                }
                stream << function;
                ++i;
            }
            return stream;
        }
    };

    inline std::ostream&
    operator<<(std::ostream& stream, const FunctionCall& self) {
        stream << "call " << self.function().name() << "(";
        std::size_t i = 0;
        for (auto& arg : self.args()) {
            if (i > 0) {
                stream << ", ";
            }
            stream << arg;
            ++i;
        }
        stream << ")";
        return stream;
    }
}
