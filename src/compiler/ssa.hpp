#pragma once
#include "../typedefs.hpp"
#include "../utils.hpp"
#include "java.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <list>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace fish::java::ssa {
    class Instruction;
    class Function;
    class BasicBlock;

    using InstructionList = std::list<Instruction>;
    using InstructionIterator = InstructionList::iterator;
    using ConstInstructionIterator = InstructionList::const_iterator;

    using java::Variable;
    using java::Constant;
    using java::ArithmeticOperator;
    using java::ComparisonOperator;

    class Move;
    class BinaryOperation;
    class Comparison;
    class UnconditionalBranch;
    class Branch;
    class ReturnVoid;
    class Return;
    class FunctionCall;
    class StandardCall;
    class Phi;
    class Load;
    class Store;
    class LoadArgument;
}

namespace fish::java::ssa::variants {
    using Terminator = std::variant<
        UnconditionalBranch,
        Branch,
        ReturnVoid,
        Return
    >;

    using Instruction = std::variant<
        Move,
        BinaryOperation,
        Comparison,
        FunctionCall,
        StandardCall,
        Phi,
        Load,
        Store,
        LoadArgument
    >;

    using Value = std::variant<
        std::monostate,
        Constant,
        InstructionIterator
    >;
}

namespace fish::java::ssa {
    inline constexpr std::monostate mono = {};

    class Value : private utils::VariantWrapper<variants::Value> {
        public:
        using VariantWrapper::VariantWrapper;
        using VariantWrapper::visit;
        using VariantWrapper::get;
        using VariantWrapper::get_if;

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const Value& self);
    };

    class UnaryInst {
        protected:
        UnaryInst() = default;

        template <typename T>
        UnaryInst(T&& value) : m_value(std::forward<T>(value)) {
        }

        public:
        auto& value() {
            return m_value;
        }

        auto& value() const {
            return m_value;
        }

        private:
        Value m_value;
    };

    class Move : public UnaryInst {
        public:
        using UnaryInst::UnaryInst;

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const Move& self) {
            stream << self.value();
            return stream;
        }
    };

    class BinaryInst {
        protected:
        BinaryInst() = default;

        template <typename Left, typename Right>
        BinaryInst(Left&& left, Right&& right) :
        m_left(std::forward<Left>(left)),
        m_right(std::forward<Right>(right)) {
        }

        public:
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
        Value m_left;
        Value m_right;
    };

    class BinaryOperation : public BinaryInst {
        public:
        using Op = ArithmeticOperator;

        BinaryOperation(Op op) : m_op(op) {
        }

        template <typename Left, typename Right>
        BinaryOperation(Op op, Left&& left, Right&& right) :
        BinaryInst(std::forward<Left>(left), std::forward<Right>(right)),
        m_op(op) {
        }

        Op& op() {
            return m_op;
        }

        const Op& op() const {
            return m_op;
        }

        private:
        Op m_op = {};

        friend std::ostream&
        operator<<(std::ostream& stream, const BinaryOperation& self) {
            stream << self.left() << " " << self.op() << " " << self.right();
            return stream;
        }
    };

    class Comparison : public BinaryInst {
        public:
        using Op = ComparisonOperator;

        Comparison(Op op) : m_op(op) {
        }

        template <typename Left, typename Right>
        Comparison(Op op, Left&& left, Right&& right) :
        BinaryInst(std::forward<Left>(left), std::forward<Right>(right)),
        m_op(op) {
        }

        Op& op() {
            return m_op;
        }

        const Op& op() const {
            return m_op;
        }

        private:
        Op m_op = {};

        friend std::ostream&
        operator<<(std::ostream& stream, const Comparison& self) {
            stream << self.left() << " " << self.op() << " " << self.right();
            return stream;
        }
    };

    class UnconditionalBranch {
        public:
        UnconditionalBranch(BasicBlock& target) : m_targets({&target}) {
        }

        BasicBlock& target() {
            auto ptr = m_targets[0];
            assert(ptr);
            return *ptr;
        }

        const BasicBlock& target() const {
            return const_cast<UnconditionalBranch&>(*this).target();
        }

        auto& successors() {
            return m_targets;
        }

        auto& successors() const {
            return m_targets;
        }

        private:
        std::array<BasicBlock*, 1> m_targets = {nullptr};

        friend std::ostream&
        operator<<(std::ostream& stream, const UnconditionalBranch& self);
    };

    class Branch {
        public:
        template <typename Cond>
        Branch(Cond&& cond, BasicBlock& yes, BasicBlock& no) :
        m_cond(std::forward<Cond>(cond)), m_targets({&yes, &no}) {
        }

        Branch(BasicBlock& yes, BasicBlock& no) :
        Branch(std::monostate(), yes, no) {
        }

        auto& cond() {
            return m_cond;
        }

        auto& cond() const {
            return m_cond;
        }

        BasicBlock& yes() {
            auto ptr = m_targets[0];
            assert(ptr);
            return *ptr;
        }

        const BasicBlock& yes() const {
            return const_cast<Branch&>(*this).yes();
        }

        BasicBlock& no() {
            auto ptr = m_targets[1];
            assert(ptr);
            return *ptr;
        }

        const BasicBlock& no() const {
            return const_cast<Branch&>(*this).no();
        }

        auto& successors() {
            return m_targets;
        }

        auto& successors() const {
            return m_targets;
        }

        private:
        Value m_cond;
        std::array<BasicBlock*, 2> m_targets = {nullptr, nullptr};

        friend std::ostream&
        operator<<(std::ostream& stream, const Branch& self);
    };

    // Inheriting for empty base optimization
    class ReturnInst : private std::array<BasicBlock*, 0> {
        protected:
        using Base = std::array<BasicBlock*, 0>;

        ReturnInst() = default;

        public:
        auto& successors() {
            return static_cast<Base&>(*this);
        }

        auto& successors() const {
            return static_cast<const Base&>(*this);
        }
    };

    class ReturnVoid : public ReturnInst {
        public:
        ReturnVoid() = default;

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const ReturnVoid&) {
            stream << "return";
            return stream;
        }
    };

    class Return : public ReturnInst, public UnaryInst {
        public:
        Return() = default;

        template <typename T>
        Return(T&& value) : UnaryInst(std::forward<T>(value)) {
        }

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const Return& self) {
            stream << "return " << self.value();
            return stream;
        }
    };

    class BaseFunctionCall {
        protected:
        BaseFunctionCall() = default;

        public:
        auto& args() {
            return m_args;
        }

        auto& args() const {
            return m_args;
        }

        private:
        std::list<Value> m_args;
    };

    class FunctionCall : public BaseFunctionCall {
        public:
        FunctionCall(Function& function) : m_function(&function) {
        }

        Function& function() {
            assert(m_function);
            return *m_function;
        }

        const Function& function() const {
            return const_cast<FunctionCall&>(*this).function();
        }

        private:
        Function* m_function = nullptr;

        friend std::ostream&
        operator<<(std::ostream& stream, const FunctionCall& self);
    };

    class StandardCall : public BaseFunctionCall {
        public:
        using Kind = java::StandardCall::Kind;

        StandardCall(Kind kind) : m_kind(kind) {
        }

        Kind& kind() {
            return m_kind;
        }

        Kind kind() const {
            return m_kind;
        }

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

    class PhiPair {
        public:
        template <typename T>
        PhiPair(BasicBlock& block, T&& value) :
        m_block(&block), m_value(std::forward<T>(value)) {
        }

        BasicBlock& block() {
            assert(m_block);
            return *m_block;
        }

        const BasicBlock& block() const {
            return const_cast<PhiPair&>(*this).block();
        }

        auto& value() {
            return m_value;
        }

        auto& value() const {
            return m_value;
        }

        private:
        BasicBlock* m_block = nullptr;
        Value m_value;

        friend std::ostream&
        operator<<(std::ostream& stream, const PhiPair& self);
    };

    class Phi {
        using Container = std::list<PhiPair>;

        public:
        using value_type = Container::value_type;
        using iterator = Container::iterator;

        Phi() = default;

        std::size_t size() const {
            return m_pairs.size();
        }

        auto begin() {
            return m_pairs.begin();
        }

        auto begin() const {
            return m_pairs.begin();
        }

        auto end() {
            return m_pairs.end();
        }

        auto end() const {
            return m_pairs.end();
        }

        void add(const PhiPair& pair) {
            emplace(pair);
        }

        template <typename... Args>
        void emplace(Args&&... args) {
            m_pairs.emplace_back(std::forward<Args>(args)...);
        }

        void erase(iterator pos) {
            m_pairs.erase(pos);
        }

        private:
        Container m_pairs;

        friend std::ostream&
        operator<<(std::ostream& stream, const Phi& self) {
            stream << "phi ";
            std::size_t i = 0;
            for (auto& pair : self) {
                if (i > 0) {
                    stream << ", ";
                }
                stream << pair;
                ++i;
            }
            return stream;
        }
    };

    class Load {
        public:
        Load(std::size_t index) : m_index(index) {
        }

        std::size_t& index() {
            return m_index;
        }

        std::size_t index() const {
            return m_index;
        }

        private:
        std::size_t m_index = 0;

        friend std::ostream&
        operator<<(std::ostream& stream, const Load& self) {
            stream << "load [" << self.index() << "]";
            return stream;
        }
    };

    // FIXME: Should only take an instruction as an argument
    class Store : public UnaryInst {
        public:
        template <typename T>
        Store(std::size_t index, T&& value) :
        UnaryInst(std::forward<T>(value)), m_index(index) {
        }

        std::size_t& index() {
            return m_index;
        }

        std::size_t index() const {
            return m_index;
        }

        private:
        std::size_t m_index = 0;

        friend std::ostream&
        operator<<(std::ostream& stream, const Store& self) {
            stream << "store [" << self.index() << "]";
            stream << ", " << self.value();
            return stream;
        }
    };

    class LoadArgument {
        public:
        LoadArgument(std::size_t index) : m_index(index) {
        }

        std::size_t& index() {
            return m_index;
        }

        std::size_t index() const {
            return m_index;
        }

        private:
        std::size_t m_index = 0;

        friend std::ostream&
        operator<<(std::ostream& stream, const LoadArgument& self) {
            stream << "load arg_" << self.index();
            return stream;
        }
    };

    template <typename Variant>
    class BlockChild : private utils::VariantWrapper<Variant> {
        public:
        using Base = utils::VariantWrapper<Variant>;

        template <typename T>
        BlockChild(BasicBlock& block, T&& obj) :
        Base(std::forward<T>(obj)), m_block(&block) {
        }

        using Base::visit;
        using Base::get;
        using Base::get_if;

        BasicBlock& block() {
            assert(m_block);
            return *m_block;
        }

        const BasicBlock& block() const {
            return const_cast<BlockChild&>(*this).block();
        }

        private:
        BasicBlock* m_block = nullptr;
    };

    class Instruction : public BlockChild<variants::Instruction> {
        public:
        using BlockChild::BlockChild;

        std::size_t id() const {
            return m_id;
        }

        std::list<Value*> inputs();
        bool has_side_effect() const;

        private:
        static inline std::size_t s_id = 0;
        std::size_t m_id = s_id++;

        friend std::ostream&
        operator<<(std::ostream& stream, const Instruction& self) {
            stream << "%" << self.id() << " = ";
            self.visit([&] (auto& obj) {
                stream << obj;
            });
            return stream;
        }
    };

    inline std::list<Value*> Instruction::inputs() {
        std::list<Value*> result;
        visit([&] (auto& obj) {
            using T = std::decay_t<decltype(obj)>;
            if constexpr (std::is_same_v<T, Move>) {
                result.push_back(&obj.value());
            }
            else if constexpr (std::is_same_v<T, BinaryOperation>) {
                result.push_back(&obj.left());
                result.push_back(&obj.right());
            }
            else if constexpr (std::is_same_v<T, Comparison>) {
                result.push_back(&obj.left());
                result.push_back(&obj.right());
            }
            else if constexpr (std::is_same_v<T, FunctionCall>) {
                for (auto& arg : obj.args()) {
                    result.push_back(&arg);
                }
            }
            else if constexpr (std::is_same_v<T, StandardCall>) {
                for (auto& arg : obj.args()) {
                    result.push_back(&arg);
                }
            }
            else if constexpr (std::is_same_v<T, Phi>) {
                for (auto& pair : obj) {
                    result.push_back(&pair.value());
                }
            }
            else if constexpr (std::is_same_v<T, Load>) {
                // Nothing
            }
            else if constexpr (std::is_same_v<T, Store>) {
                result.push_back(&obj.value());
            }
            else if constexpr (std::is_same_v<T, LoadArgument>) {
                // Nothing
            }
            else {
                static_assert(utils::always_false<T>);
            }
        });
        return result;
    }

    inline bool Instruction::has_side_effect() const {
        return visit([&] (auto& obj) -> bool {
            using T = std::decay_t<decltype(obj)>;
            if constexpr (std::is_same_v<T, Move>) {
                return false;
            }
            else if constexpr (std::is_same_v<T, BinaryOperation>) {
                return false;
            }
            else if constexpr (std::is_same_v<T, Comparison>) {
                return false;
            }
            else if constexpr (std::is_same_v<T, FunctionCall>) {
                return true;
            }
            else if constexpr (std::is_same_v<T, StandardCall>) {
                return true;
            }
            else if constexpr (std::is_same_v<T, Phi>) {
                return false;
            }
            else if constexpr (std::is_same_v<T, Load>) {
                return false;
            }
            else if constexpr (std::is_same_v<T, Store>) {
                return true;
            }
            else if constexpr (std::is_same_v<T, LoadArgument>) {
                return false;
            }
            else {
                static_assert(utils::always_false<T>);
                return false;
            }
        });
    }

    class Terminator : public BlockChild<variants::Terminator> {
        public:
        using BlockChild::BlockChild;

        std::list<Value*> inputs() {
            std::list<Value*> result;
            visit([&] (auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                if constexpr (std::is_same_v<T, UnconditionalBranch>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Branch>) {
                    result.push_back(&obj.cond());
                }
                else if constexpr (std::is_same_v<T, ReturnVoid>) {
                    // Nothing
                }
                else if constexpr (std::is_same_v<T, Return>) {
                    result.push_back(&obj.value());
                }
                else {
                    static_assert(utils::always_false<T>);
                }
            });
            return result;
        }

        private:
        friend std::ostream&
        operator<<(std::ostream& stream, const Terminator& self) {
            self.visit([&] (auto& obj) {
                stream << obj;
            });
            return stream;
        }
    };

    class BasicBlock {
        public:
        template <typename Self>
        class InstructionView {
            public:
            using iterator = InstructionIterator;
            using const_iterator = ConstInstructionIterator;

            InstructionView(Self& self) : m_self(self) {
            }

            auto begin() {
                return m_self.m_instructions.begin();
            }

            auto end() {
                return m_self.m_instructions.end();
            }

            template <typename T>
            auto insert(InstructionIterator pos, T&& inst) {
                return m_self.m_instructions.insert(
                    pos, Instruction(m_self, std::forward<T>(inst))
                );
            }

            template <typename T>
            auto prepend(T&& inst) {
                return insert(begin(), std::forward<T>(inst));
            }

            template <typename T>
            auto append(T&& inst) {
                return insert(end(), std::forward<T>(inst));
            }

            auto erase(InstructionIterator pos) {
                return m_self.m_instructions.erase(pos);
            }

            private:
            Self& m_self;
        };

        BasicBlock() = default;

        auto instructions() {
            return InstructionView(*this);
        }

        auto instructions() const {
            return InstructionView(*this);
        }

        Terminator& terminator() {
            assert(m_terminator);
            return *m_terminator;
        }

        const Terminator& terminator() const {
            return const_cast<BasicBlock&>(*this).terminator();
        }

        auto& terminator(std::nullopt_t) {
            return m_terminator;
        }

        template <typename T>
        Terminator& terminate(T&& terminator) {
            m_terminator = Terminator(*this, std::forward<T>(terminator));
            auto& term = m_terminator->template get<std::decay_t<T>>();
            clear_successors();
            for (BasicBlock* block : term.successors()) {
                add_successor(*block);
            }
            return *m_terminator;
        }

        const auto& predecessors() const {
            return m_predecessors;
        }

        const auto& successors() const {
            return m_successors;
        }

        std::size_t id() const {
            return m_id;
        }

        std::vector<std::pair<InstructionIterator, Value*>>
        phis(BasicBlock& block) {
            std::vector<std::pair<InstructionIterator, Value*>> result;
            auto it = instructions().begin();
            auto end = instructions().end();

            for (; it != end; ++it) {
                auto& inst = *it;
                auto ptr = inst.get_if<Phi>();
                if (!ptr) {
                    break;
                }

                Phi& phi = *ptr;
                for (PhiPair& pair : phi) {
                    if (&pair.block() == &block) {
                        result.emplace_back(it, &pair.value());
                        break;
                    }
                }
            }
            return result;
        }

        protected:
        friend class Function;

        void unlink() {
            unlink_predecessors();
            unlink_successors();
        }

        private:
        static inline std::size_t s_id = 0;
        std::size_t m_id = s_id++;

        InstructionList m_instructions;
        std::optional<Terminator> m_terminator;
        std::set<BasicBlock*> m_predecessors;
        std::set<BasicBlock*> m_successors;

        void add_successor(BasicBlock& block) {
            [[maybe_unused]] bool inserted;
            inserted = m_successors.insert(&block).second;
            assert(inserted);
            inserted = block.m_predecessors.insert(this).second;
            assert(inserted);
        }

        void clear_successors() {
            unlink_successors();
            m_successors.clear();
        }

        void unlink_predecessors() {
            [[maybe_unused]] std::size_t removed = 0;
            for (BasicBlock* block : m_predecessors) {
                removed = block->m_successors.erase(this);
                assert(removed == 1);
            }
        }

        void unlink_successors() {
            [[maybe_unused]] std::size_t removed = 0;
            for (BasicBlock* block : m_successors) {
                removed = block->m_predecessors.erase(this);
                assert(removed == 1);
            }
        }

        friend std::ostream&
        operator<<(std::ostream& stream, const BasicBlock& self) {
            stream << "block @" << self.id() << "\n";
            {
                stream << "pred(";
                std::size_t i = 0;
                for (const BasicBlock* pred : self.predecessors()) {
                    if (i > 0) {
                        stream << ", ";
                    }
                    stream << pred->id();
                    ++i;
                }
                stream << ")\n";
            }

            {
                stream << "succ(";
                std::size_t i = 0;
                for (const BasicBlock* succ : self.successors()) {
                    if (i > 0) {
                        stream << ", ";
                    }
                    stream << succ->id();
                    ++i;
                }
                stream << ") {\n";
            }

            for ([[maybe_unused]] auto& instruction : self.instructions()) {
                stream << utils::indent(utils::str(instruction), 4) << "\n";
            }
            if (self.m_terminator) {
                stream << utils::indent(utils::str(self.terminator()), 4);
                stream << "\n";
            }

            stream << "}";
            return stream;
        }
    };

    class Function {
        public:
        using BlockList = std::list<BasicBlock>;
        using BlockIterator = BlockList::iterator;

        template <typename Self>
        class BlockView {
            public:
            using iterator = BlockIterator;

            BlockView(Self& self) : m_self(self) {
            }

            auto begin() {
                return m_self.m_blocks.begin();
            }

            auto end() {
                return m_self.m_blocks.end();
            }

            auto insert(iterator pos, const BasicBlock& block) {
                return m_self.m_blocks.insert(pos, block);
            }

            auto append(const BasicBlock& block) {
                return insert(end(), block);
            }

            auto erase(iterator pos) {
                pos->unlink();
                return m_self.m_blocks.erase(pos);
            }

            private:
            Self& m_self;
        };

        Function(std::size_t nargs, std::size_t nreturn, std::string name) :
        m_nargs(nargs), m_nreturn(nreturn), m_name(std::move(name)) {
        }

        std::size_t& nargs() {
            return m_nargs;
        }

        std::size_t nargs() const {
            return m_nargs;
        }

        std::size_t& nreturn() {
            return m_nreturn;
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

        auto blocks() {
            return BlockView(*this);
        }

        auto blocks() const {
            return BlockView(*this);
        }

        std::size_t& stack_slots() {
            return m_stack_slots;
        }

        std::size_t stack_slots() const {
            return m_stack_slots;
        }

        private:
        std::size_t m_nargs = 0;
        std::size_t m_nreturn = 0;
        std::string m_name;
        std::list<BasicBlock> m_blocks;
        std::size_t m_stack_slots = 0;

        friend std::ostream&
        operator<<(std::ostream& stream, const Function& self) {
            stream << "function " << self.name() << " ";
            stream << "(" << self.nargs() << ") {\n";
            std::size_t i = 0;
            for (auto& block : self.blocks()) {
                if (i > 0) {
                    stream << "\n";
                }
                stream << utils::indent(utils::str(block), 4) << "\n";
                ++i;
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
    operator<<(std::ostream& stream, const Value& self) {
        self.visit([&] (auto& obj) {
            using T = std::decay_t<decltype(obj)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                stream << "<empty>";
            }
            else if constexpr (std::is_same_v<T, InstructionIterator>) {
                stream << "%" << obj->id();
            }
            else {
                stream << obj;
            }
        });
        return stream;
    }

    inline std::ostream&
    operator<<(std::ostream& stream, const UnconditionalBranch& self) {
        stream << "goto @" << self.target().id();
        return stream;
    }

    inline std::ostream&
    operator<<(std::ostream& stream, const Branch& self) {
        stream << "goto " << self.m_cond << " ? @" << self.yes().id();
        stream << " : @" << self.no().id();
        return stream;
    }

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

    inline std::ostream&
    operator<<(std::ostream& stream, const PhiPair& self) {
        stream << "[@" << self.block().id() << ", " << self.value() << "]";
        return stream;
    }
}
