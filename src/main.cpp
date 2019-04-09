#include "class-file.hpp"
#include "stream.hpp"
#include <fstream>
#include <iostream>
#include <istream>
#include <stdexcept>

using namespace Fish::Java;

int main() {
    std::ifstream is("Test.class");
    Stream stream(is);
    ClassFile cls(stream);
    if (is.eof()) {
        throw std::runtime_error("Unexpected EOF");
    }
    is.get();
    if (!is.eof()) {
        throw std::runtime_error("Extra data");
    }
    std::cout << "Processed " << stream.pos() << " bytes\n";
    return 0;
}
