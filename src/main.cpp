#include "class-file.hpp"
#include <fstream>
#include <iostream>
#include <istream>
#include <stdexcept>

using namespace Fish::Java;

int main() {
    std::ifstream stream("Test.class");
    ClassFile cls(stream);
    if (stream.eof()) {
        throw std::runtime_error("Unexpected EOF");
    }
    stream.get();
    if (!stream.eof()) {
        throw std::runtime_error("Extra data");
    }
    return 0;
}
