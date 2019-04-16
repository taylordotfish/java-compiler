#pragma once
#include "../code-info.hpp"
#include "../class-file.hpp"
#include "../constant-pool.hpp"
#include "../opcode.hpp"
#include "../typedefs.hpp"
#include "../utils.hpp"
#include "java.hpp"
#include <cassert>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <vector>

namespace fish::java::java {
    class ProgramBuilder {
        public:
        ProgramBuilder(Program& program, const ClassFile& cls) :
        m_program(program), m_cls(cls) {
            const auto& methods = cls.methods;
            for (const MethodInfo& minfo : methods) {
                const auto& descriptor = minfo.descriptor(cpool());
                Function& func = *m_program.functions().add(Function(
                    descriptor.nargs(), descriptor.nreturn(),
                    minfo.name(cpool())
                ));
                m_funcs.emplace(minfo.name_and_type(), &func);
            }
        }

        void build();

        protected:
        friend class FunctionBuilder;

        Function& find_function(const pool::NameAndType& id) {
            auto it = m_funcs.find(id);
            assert(it != m_funcs.end());
            return *it->second;
        }

        const ClassFile& cls() const {
            return m_cls;
        }

        private:
        Program& m_program;
        const ClassFile& m_cls;
        std::map<pool::NameAndType, Function*> m_funcs;

        const ConstantPool& cpool() const {
            return cls().cpool;
        }
    };

    class FunctionBuilder {
        protected:
        friend class InstructionBuilder;
        using InstIter = InstructionIterator;
        using ConstInstIter = ConstInstructionIterator;
        using OptInstIter = std::optional<InstIter>;

        class InstRef {
            public:
            InstRef(const u8* code, OptInstIter& inst, u64 depth) :
            m_code(code), m_inst(&inst), m_depth(depth) {
            }

            const u8* code() const {
                return m_code;
            }

            OptInstIter& inst() {
                assert(m_inst);
                return *m_inst;
            }

            u64 depth() const {
                return m_depth;
            }

            private:
            const u8* m_code = nullptr;
            OptInstIter* m_inst = nullptr;
            u64 m_depth = 0;
        };

        public:
        FunctionBuilder(
            ProgramBuilder& parent, Function& func, const MethodInfo& minfo
        ) :
        m_parent(parent), m_function(func), m_minfo(minfo) {
        }

        void build();

        protected:
        u64& depth() {
            return m_depth;
        }

        void bind(OptInstIter& inst, const u8* code) {
            m_unlinked.emplace_back(code, inst, depth());
        }

        template <typename T>
        decltype(auto) append(T&& inst) {
            auto it = function().instructions().append(std::forward<T>(inst));
            for (const u8* code : m_sources) {
                m_inst_map.emplace(code, it);
            }
            m_sources.clear();
            return it;
        }

        const ClassFile& cls() const {
            return m_parent.cls();
        }

        Function& find_function(const pool::NameAndType& id) {
            return m_parent.find_function(id);
        }

        private:
        ProgramBuilder& m_parent;
        Function& m_function;
        const MethodInfo& m_minfo;

        std::unordered_map<const u8*, InstructionIterator> m_inst_map;
        std::list<InstRef> m_unlinked;
        std::vector<const u8*> m_sources;
        u64 m_depth = -1;

        void build_at_pos(const u8* code);

        Function& function() {
            return m_function;
        }

        const CodeSeq& code() const {
            const CodeInfo& code_info = m_minfo.code;
            return code_info.code;
        }

        const u8* code_begin() const {
            return &code()[0];
        }

        const u8* code_end() const {
            return &code()[code().size()];
        }
    };

    inline void ProgramBuilder::build() {
        const auto& methods = cls().methods;
        for (const MethodInfo& minfo : methods) {
            Function& func = find_function(minfo.name_and_type());
            FunctionBuilder builder(*this, func, minfo);
            builder.build();
        }
    }

    class InstructionBuilder {
        public:
        InstructionBuilder(FunctionBuilder& parent, const u8* code) :
        m_parent(parent), m_code(code) {
        }

        u64 build();

        private:
        using OptInstIter = FunctionBuilder::OptInstIter;

        FunctionBuilder& m_parent;
        const u8* m_code = nullptr;

        template <typename T>
        decltype(auto) emit(T&& inst) {
            return m_parent.append(std::forward<T>(inst));
        }

        Variable push() {
            return Variable(Variable::stack, ++depth());
        }

        template <typename T>
        decltype(auto) push(T&& source) {
            return emit(Move(std::forward<T>(source), push()));
        }

        decltype(auto) push_local(u32 index) {
            return push(Variable(Variable::locals, index));
        }

        decltype(auto) push_const(u64 value) {
            return push(Constant(value));
        }

        Variable pop() {
            return Variable(Variable::stack, depth()--);
        }

        decltype(auto) pop(Variable dest) {
            return emit(Move(pop(), dest));
        }

        decltype(auto) pop_local(u32 index) {
            return pop(Variable(Variable::locals, index));
        }

        u64& depth() {
            return m_parent.depth();
        }

        void bind(OptInstIter& inst, const u8* code) {
            m_parent.bind(inst, code);
        }

        const ClassFile& cls() const {
            return m_parent.cls();
        }

        Function& find_function(const pool::NameAndType& id) {
            return m_parent.find_function(id);
        }

        void binary_op(BinaryOperation::Op op);
        u64 build_icmp();
        u64 build_if();
        u64 build_invokestatic();
        u64 build_invokevirtual();
        void emit_print(const MethodDescriptor& mdesc);
        void emit_println(const MethodDescriptor& mdesc);

    };

    inline void FunctionBuilder::build() {
        const u8* code = code_begin();
        build_at_pos(code);

        while (!m_unlinked.empty()) {
            auto ref = m_unlinked.front();
            m_unlinked.pop_front();

            auto map_iter = m_inst_map.find(ref.code());
            if (map_iter == m_inst_map.end()) {
                depth() = ref.depth();
                build_at_pos(ref.code());
                map_iter = m_inst_map.find(ref.code());
            }

            assert(map_iter != m_inst_map.end());
            auto inst_iter = map_iter->second;
            ref.inst() = inst_iter;
            inst_iter->flags().target() = true;
        }
    }

    inline void FunctionBuilder::build_at_pos(const u8* code) {
        [[maybe_unused]] const u8* end = code_end();
        while (true) {
            m_sources.push_back(code);
            InstructionBuilder builder(*this, code);
            u64 inc = builder.build();
            if (inc == 0) {
                break;
            }
            code += inc;
            assert(code < end);
        }
    }

    inline u64 InstructionBuilder::build() {
        const u8* code = m_code;
        switch (static_cast<Opcode>(*code)) {
            case Opcode::iconst_m1:
            case Opcode::iconst_0:
            case Opcode::iconst_1:
            case Opcode::iconst_2:
            case Opcode::iconst_3:
            case Opcode::iconst_4:
            case Opcode::iconst_5: {
                push_const(
                    static_cast<s32>(*code) -
                    static_cast<s32>(Opcode::iconst_0)
                );
                return 1;
            }

            case Opcode::iload: {
                u8 index = code[1];
                push_local(index);
                return 2;
            }

            case Opcode::iload_0:
            case Opcode::iload_1:
            case Opcode::iload_2:
            case Opcode::iload_3: {
                push_local(
                    static_cast<s32>(*code) -
                    static_cast<s32>(Opcode::iload_0)
                );
                return 1;
            }

            case Opcode::istore: {
                u8 index = code[1];
                pop_local(index);
                return 2;
            }

            case Opcode::istore_0:
            case Opcode::istore_1:
            case Opcode::istore_2:
            case Opcode::istore_3: {
                pop_local(
                    static_cast<s32>(*code) -
                    static_cast<s32>(Opcode::istore_0)
                );
                return 1;
            }

            case Opcode::iinc: {
                u8 index = code[1];
                s8 amount = static_cast<s8>(code[2]);
                Variable v1 = Variable(Variable::locals, index);
                emit(BinaryOperation(
                    BinaryOperation::Op::add, v1, Constant(amount), v1
                ));
                return 3;
            }

            case Opcode::iadd: {
                binary_op(BinaryOperation::Op::add);
                return 1;
            }

            case Opcode::isub: {
                binary_op(BinaryOperation::Op::sub);
                return 1;
            }

            case Opcode::imul: {
                binary_op(BinaryOperation::Op::mul);
                return 1;
            }

            case Opcode::ishl: {
                binary_op(BinaryOperation::Op::shl);
                return 1;
            }

            case Opcode::ishr: {
                binary_op(BinaryOperation::Op::shr);
                return 1;
            }

            case Opcode::if_icmpeq:
            case Opcode::if_icmpne:
            case Opcode::if_icmpgt:
            case Opcode::if_icmpge:
            case Opcode::if_icmplt:
            case Opcode::if_icmple: {
                return build_icmp();
            }

            case Opcode::ifeq:
            case Opcode::ifne:
            case Opcode::ifgt:
            case Opcode::ifge:
            case Opcode::iflt:
            case Opcode::ifle: {
                return build_if();
            }

            case Opcode::Goto: {
                auto it = emit(UnconditionalBranch());
                auto& branch = it->get<UnconditionalBranch>();
                s16 offset = static_cast<s16>(code[1] << 8 | code[2]);
                bind(branch.target(std::nullopt), code + offset);
                return 0;
            }

            case Opcode::bipush: {
                push_const(static_cast<s8>(code[1]));
                return 2;
            }

            case Opcode::sipush: {
                push_const(static_cast<s16>(code[1] << 8 | code[2]));
                return 3;
            }

            case Opcode::invokestatic: {
                return build_invokestatic();
            }

            case Opcode::invokevirtual: {
                return build_invokevirtual();
            }

            case Opcode::Return: {
                emit(ReturnVoid());
                return 0;
            }

            case Opcode::ireturn: {
                Variable v1 = pop();
                emit(Return(v1));
                return 0;
            }

            case Opcode::getstatic: {
                // NOTE: Ignoring object
                // push_const(0);
                return 3;
            }

            case Opcode::pop: {
                pop();
                return 1;
            }

            default: {
                std::ostringstream msg;
                msg << "Unsupported opcode: 0x";
                msg << std::hex << static_cast<int>(*code);
                throw std::runtime_error(msg.str());
            }
        }
    }

    inline void InstructionBuilder::binary_op(BinaryOperation::Op op) {
        Variable v2 = pop();
        Variable v1 = pop();
        emit(BinaryOperation(op, v1, v2, push()));
    }

    static inline Branch::Op op_from_icmp(Opcode icmp_op) {
        switch (icmp_op) {
            case Opcode::if_icmpeq: {
                return Branch::Op::eq;
            }
            case Opcode::if_icmpne: {
                return Branch::Op::ne;
            }
            case Opcode::if_icmpgt: {
                return Branch::Op::gt;
            }
            case Opcode::if_icmpge: {
                return Branch::Op::ge;
            }
            case Opcode::if_icmplt: {
                return Branch::Op::lt;
            }
            case Opcode::if_icmple: {
                return Branch::Op::le;
            }
            default: {
                std::ostringstream msg;
                msg << "Invalid `if_icmp` opcode: 0x";
                msg << std::hex << static_cast<int>(icmp_op);
                throw std::runtime_error(msg.str());
            }
        }
    }

    static inline Branch::Op op_from_if(Opcode if_op) {
        switch (if_op) {
            case Opcode::ifeq: {
                return Branch::Op::eq;
            }
            case Opcode::ifne: {
                return Branch::Op::ne;
            }
            case Opcode::ifgt: {
                return Branch::Op::gt;
            }
            case Opcode::ifge: {
                return Branch::Op::ge;
            }
            case Opcode::iflt: {
                return Branch::Op::lt;
            }
            case Opcode::ifle: {
                return Branch::Op::le;
            }
            default: {
                std::ostringstream msg;
                msg << "Invalid `if` opcode: 0x";
                msg << std::hex << static_cast<int>(if_op);
                throw std::runtime_error(msg.str());
            }
        }
    }

    inline u64 InstructionBuilder::build_icmp() {
        const u8* code = m_code;
        Variable v2 = pop();
        Variable v1 = pop();
        Branch::Op op = op_from_icmp(static_cast<Opcode>(*code));
        auto it = emit(Branch(op, v1, v2));
        auto& branch = it->get<Branch>();
        s16 offset = static_cast<s16>(code[1] << 8 | code[2]);
        bind(branch.target(std::nullopt), code + offset);
        return 3;
    }

    inline u64 InstructionBuilder::build_if() {
        const u8* code = m_code;
        Variable v1 = pop();
        Branch::Op op = op_from_if(static_cast<Opcode>(*code));
        auto it = emit(Branch(op, v1, Constant(0)));
        auto& branch = it->get<Branch>();
        s16 offset = static_cast<s16>(code[1] << 8 | code[2]);
        bind(branch.target(std::nullopt), code + offset);
        return 3;
    }

    inline u64 InstructionBuilder::build_invokestatic() {
        const u8* code = m_code;
        const u16 index = code[1] << 8 | code[2];
        const ClassFile& cls = this->cls();
        const ConstantPool& cpool = cls.cpool;

        auto& name_and_type = cpool[index].visit(
            [&] (auto& obj) -> const pool::NameAndType& {
                constexpr bool is_method_ref = std::is_base_of_v<
                    pool::BaseMethodRef, std::decay_t<decltype(obj)>
                >;
                if constexpr (!is_method_ref) {
                    throw std::runtime_error(
                        "Expected method entry in constant pool"
                    );
                }
                else if (obj.class_ref_index != cls.self_index) {
                    throw std::runtime_error(
                        "Cannot call method of other class"
                    );
                }
                else {
                    return cpool.get<pool::NameAndType>(
                        obj.name_type_index
                    );
                }
            }
        );

        Function& func = find_function(name_and_type);
        auto& sig = cpool.get<pool::UTF8>(name_and_type.desc_index).str;
        MethodDescriptor mdesc(sig);

        auto it = emit(FunctionCall(func));
        FunctionCall& call = it->get<FunctionCall>();

        for (std::size_t i = 0; i < mdesc.nargs(); ++i) {
            call.args().emplace_front(pop());
        }

        if (mdesc.nargs() > 0) {
            call.dest().emplace(push());
        }
        return 3;
    }

    inline u64 InstructionBuilder::build_invokevirtual() {
        const u8* code = m_code;
        const u16 index = code[1] << 8 | code[2];
        const ClassFile& cls = this->cls();
        const ConstantPool& cpool = cls.cpool;

        auto& name_and_type = cpool[index].visit(
            [&] (auto& obj) -> const pool::NameAndType& {
                constexpr bool is_method_ref = std::is_base_of_v<
                    pool::BaseMethodRef, std::decay_t<decltype(obj)>
                >;
                if constexpr (!is_method_ref) {
                    throw std::runtime_error(
                        "Expected method entry in constant pool"
                    );
                }
                else {
                    return cpool.get<pool::NameAndType>(
                        obj.name_type_index
                    );
                }
            }
        );

        auto& name = cpool.get<pool::UTF8>(name_and_type.name_index).str;
        auto& sig = cpool.get<pool::UTF8>(name_and_type.desc_index).str;
        MethodDescriptor mdesc(sig);

        if (name != "print" && name != "println") {
            std::ostringstream msg;
            msg << "Unsupported virtual method: " << name;
            throw std::runtime_error(msg.str());
        }

        utils::check_print_method_descriptor(mdesc, name + "()");
        if (name == "print") {
            emit_print(mdesc);
        } else {
            emit_println(mdesc);
        }
        return 3;
    }

    inline void
    InstructionBuilder::emit_print(const MethodDescriptor& mdesc) {
        using Kind = StandardCall::Kind;

        if (mdesc.nargs() == 0) {
            throw std::runtime_error("print() must take an argument.");
        }
        else if (mdesc.arg(0) == "C") {
            emit(StandardCall(Kind::print_char, pop()));
        }
        else {
            emit(StandardCall(Kind::print_int, pop()));
        }
    }

    inline void
    InstructionBuilder::emit_println(const MethodDescriptor& mdesc) {
        using Kind = StandardCall::Kind;

        if (mdesc.nargs() == 0) {
            emit(StandardCall(Kind::println_void));
        }
        else if (mdesc.arg(0) == "C") {
            emit(StandardCall(Kind::println_char, pop()));
        }
        else {
            emit(StandardCall(Kind::println_int, pop()));
        }
    }
}
