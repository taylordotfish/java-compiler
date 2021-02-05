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

#include "class-file.hpp"
#include "interpreter.hpp"
#include "stream.hpp"
#include "compiler/java-build.hpp"
#include "compiler/ssa-build.hpp"
#include "compiler/x64-build.hpp"
#include "compiler/x64-assemble.hpp"
#include <sys/mman.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <stdexcept>
#include <string>

using namespace fish::java;

static constexpr const char* usage = R"(
Usage:
  compiler interpret <class-file>
  compiler compile <class-file> [<x64-out>]
  compiler ssa <class-file>
)" + 1;

static bool propagate_copies(ssa::Function& function) {
    std::map<ssa::Instruction*, ssa::Value> map;
    bool changed = false;

    auto propagate = [&] (ssa::Value& input) {
        auto input_inst = input.get_if<ssa::InstructionIterator>();
        if (!input_inst) return;
        auto map_entry = map.find(&**input_inst);
        if (map_entry == map.end()) return;
        input = ssa::Value(map_entry->second);
        changed = true;
    };

    for (ssa::BasicBlock& block : function.blocks()) {
        auto it = block.instructions().begin();
        auto end = block.instructions().end();

        for (; it != end; ++it) {
            for (ssa::Value* input : it->inputs()) {
                propagate(*input);
            }

            auto move = it->get_if<ssa::Move>();
            if (!move) continue;
            map.emplace(&*it, move->value());
        }

        for (ssa::Value* input : block.terminator().inputs()) {
            propagate(*input);
        }
    }
    return changed;
}

static bool eliminate_unused(ssa::Function& function) {
    std::set<ssa::Instruction*> used;
    for (ssa::BasicBlock& block : function.blocks()) {
        for (auto& inst : block.instructions()) {
            for (ssa::Value* input : inst.inputs()) {
                auto input_inst = input->get_if<ssa::InstructionIterator>();
                if (!input_inst) continue;
                used.insert(&**input_inst);
            }
        }

        for (ssa::Value* input : block.terminator().inputs()) {
            auto input_inst = input->get_if<ssa::InstructionIterator>();
            if (!input_inst) continue;
            used.insert(&**input_inst);
        }
    }

    std::list<ssa::InstructionIterator> remove;
    for (ssa::BasicBlock& block : function.blocks()) {
        auto it = block.instructions().begin();
        auto end = block.instructions().end();

        for (; it != end; ++it) {
            if (used.count(&*it) > 0) continue;
            if (it->has_side_effect()) continue;
            remove.push_back(it);
        }
    }

    for (auto inst : remove) {
        inst->block().instructions().erase(inst);
    }
    return !remove.empty();
}

static void optimize(ssa::Function& function) {
    static constexpr std::size_t max_rounds = 20;
    std::size_t i = 0;
    do {} while (++i <= max_rounds && (
        propagate_copies(function) ||
        eliminate_unused(function)
    ));
}

static void cls_to_ssa(const ClassFile& cls, ssa::Program& ssa_program) {
    java::Program j_program;
    auto j_builder = java::ProgramBuilder(j_program, cls);
    j_builder.build();

    auto ssa_builder = ssa::ProgramBuilder(ssa_program, j_program);
    ssa_builder.build();

    for (auto& function : ssa_program.functions()) {
        optimize(function);
    }
}

static int cmd_ssa(const ClassFile& cls, int, char**) {
    ssa::Program ssa_program;
    cls_to_ssa(cls, ssa_program);
    std::cout << ssa_program;
    return EXIT_SUCCESS;
}

static int cmd_compile(const ClassFile& cls, int argc, char** argv) {
    ssa::Program ssa_program;
    cls_to_ssa(cls, ssa_program);

    x64::Program x64_program;
    x64::ProgramBuilder x64_builder(x64_program, ssa_program);
    x64_builder.build();

    const x64::Function* entry_func = nullptr;
    for (auto& func : x64_program.functions()) {
        if (func.name() == "main") {
            entry_func = &func;
        }
    }

    if (!entry_func) {
        std::cerr << "Could not find main method.\n";
        return EXIT_FAILURE;
    }

    x64::Assembler x64_assembler(x64_program);
    x64_assembler.assemble();
    auto& code = x64_assembler.code();
    std::size_t entry_offset = x64_assembler.find(*entry_func);

    if (argc > 3) {
        std::ofstream out(argv[3]);
        if (!out.is_open()) {
            std::cerr << "Could not open x64 output file.\n";
            return EXIT_FAILURE;
        }
        for (u8 code_byte : code) {
            out.put(code_byte);
        }
        return EXIT_SUCCESS;
    }

    unsigned char* memory = static_cast<unsigned char*>(mmap(
        nullptr, code.size(), PROT_EXEC | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, 0, 0
    ));
    if (!memory) {
        std::cout << "Could not allocate x64 code buffer.\n";
        return EXIT_FAILURE;
    }
    std::memcpy(memory, &code[0], code.size());
    ((void (*)())(memory + entry_offset))();
    return EXIT_SUCCESS;
}

static int cmd_interpret(const ClassFile& cls, int, char**) {
    Interpreter interpreter(cls);
    interpreter.run();
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    if (argc <= 2) {
        std::cerr << usage;
        return EXIT_FAILURE;
    }

    std::ifstream fstream(argv[2]);
    if (!fstream.is_open()) {
        std::cerr << "Could not read class file.\n";
        return EXIT_FAILURE;
    }

    ClassFile cls(fstream);
    if (fstream.eof()) {
        throw std::runtime_error("Unexpected end of class file");
    }
    fstream.get();
    if (!fstream.eof()) {
        throw std::runtime_error("Unexpected extra data in class file");
    }

    if (argv[1] == std::string("interpret")) {
        return cmd_interpret(cls, argc, argv);
    }
    if (argv[1] == std::string("compile")) {
        return cmd_compile(cls, argc, argv);
    }
    if (argv[1] == std::string("ssa")) {
        return cmd_ssa(cls, argc, argv);
    }

    std::cerr << usage;
    return EXIT_FAILURE;
}
