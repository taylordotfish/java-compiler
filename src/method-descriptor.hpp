#pragma once
#include <cassert>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace Fish::Java {
    class MethodDescriptor {
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

        const std::string& arg(std::size_t i) const {
            assert(i < m_args.size());
            return m_args[i];
        }

        private:
        std::vector<std::string> m_args;
        std::string m_rtype;

        bool try_parse(const std::string& sig) {
            std::size_t i = 1;
            for (; i < sig.size(); ++i) {
                if (sig[i] == ')') {
                    ++i;
                    break;
                }
                switch (sig[i]) {
                    case 'I':
                    case 'C': {
                        m_args.emplace_back(1, sig[i]);
                        continue;
                    }
                }
                return false;
            }
            if (i >= sig.size()) {
                return false;
            }
            m_rtype = std::string(1, sig[i]);
            return true;
        }
    };
}
