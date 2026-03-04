#include "addition/config.hpp"

namespace addition {

const ChainConfig& default_config() {
    static const ChainConfig cfg{};
    return cfg;
}

} // namespace addition
