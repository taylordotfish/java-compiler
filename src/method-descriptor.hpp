#pragma once
#include <cassert>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fish::java {
    /**
     * NOTE: Supports only certain primitive types.
     */
    class MethodDescriptor {
        // Signature of main method
        static constexpr char main[] = "([Ljava/lang/String;)V";

        public:
        MethodDescriptor(const std::string& sig) {
            if (!try_parse(sig)) {
                std::ostringstream msg;
                msg << "Unsupported method descriptor: " << sig;
                throw std::runtime_error(msg.str());
            }
        }

        std::string rtype() const {
            return m_rtype;
        }

        std::size_t nargs() const {
            return m_args.size();
        }

        std::size_t nreturn() const {
            return rtype() == "V" ? 0 : 1;
        }

        const std::string& arg(std::size_t i) const {
            assert(i < m_args.size());
            return m_args[i];
        }

        private:
        std::vector<std::string> m_args;
        std::string m_rtype;

        bool is_primitive_type(char c) {
            switch (c) {
                case 'V':
                case 'I':
                case 'B':
                case 'C':
                case 'S':
                case 'Z': {
                    return true;
                }
                default: {
                    return false;
                }
            }
        }

        bool try_parse(const std::string& sig) {
            // NOTE: Pretending that main() takes no arguments.
            if (sig == main) {
                m_rtype = "V";
                return true;
            }

            std::size_t i = 1;
            for (; i < sig.size(); ++i) {
                if (sig[i] == ')') {
                    ++i;
                    break;
                }
                if (!is_primitive_type(sig[i])) {
                    return false;
                }
                m_args.emplace_back(1, sig[i]);
            }
            if (i >= sig.size()) {
                return false;
            }
            if (!is_primitive_type(sig[i])) {
                return false;
            }
            m_rtype = std::string(1, sig[i]);
            return true;
        }
    };
}
