#include "class-file.hpp"
#include "interpreter.hpp"
#include "stream.hpp"
#include <fstream>
#include <iostream>
#include <istream>
#include <stdexcept>

using namespace Fish::Java;

int main() {
    std::ifstream fstream("Test.class");
    ClassFile cls(fstream);
    if (fstream.eof()) {
        throw std::runtime_error("Unexpected EOF");
    }
    fstream.get();
    if (!fstream.eof()) {
        throw std::runtime_error("Extra data");
    }

    Interpreter interpreter(cls);
    interpreter.run();
    return 0;
}
