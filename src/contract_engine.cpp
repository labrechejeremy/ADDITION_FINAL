#include "addition/contract_engine.hpp"

#include "addition/crypto.hpp"

#include <sstream>

namespace addition {

ContractEngine::ContractEngine(TokenEngine* tokens) : tokens_(tokens) {}

void ContractEngine::bind_token_engine(TokenEngine* tokens) {
    tokens_ = tokens;
}

std::string ContractEngine::deploy(const std::string& owner, const std::string& code) {
    const auto id = to_hex(sha3_512_bytes(owner + "|" + code));
    contracts_[id] = Contract{owner, code, {}};
    return id;
}

bool ContractEngine::call(const std::string& contract_id,
                          const std::string& method,
                          const std::string& key,
                          std::int64_t value,
                          std::string& out,
                          std::string& error) {
    auto it = contracts_.find(contract_id);
    if (it == contracts_.end()) {
        error = "contract not found";
        return false;
    }

    auto& c = it->second;

    if (method == "set") {
        c.kv[key] = value;
        out = "ok";
        return true;
    }

    if (method == "add") {
        c.kv[key] += value;
        out = std::to_string(c.kv[key]);
        return true;
    }

    if (method == "get") {
        const auto found = c.kv.find(key);
        const auto val = (found == c.kv.end()) ? 0 : found->second;
        out = std::to_string(val);
        return true;
    }

    if (method == "token_balance") {
        if (tokens_ == nullptr) {
            error = "token engine not bound";
            return false;
        }
        const auto sep = key.find(':');
        if (sep == std::string::npos || sep == 0 || sep + 1 >= key.size()) {
            error = "token_balance key format: <SYMBOL>:<OWNER>";
            return false;
        }
        const auto symbol = key.substr(0, sep);
        const auto owner = key.substr(sep + 1);
        out = std::to_string(tokens_->balance_of(symbol, owner));
        return true;
    }

    if (method == "swap_quote") {
        if (tokens_ == nullptr) {
            error = "token engine not bound";
            return false;
        }
        const auto sep = key.find(':');
        if (sep == std::string::npos || sep == 0 || sep + 1 >= key.size()) {
            error = "swap_quote key format: <TOKEN_IN>:<TOKEN_OUT>";
            return false;
        }
        if (value <= 0) {
            error = "swap_quote value must be > 0";
            return false;
        }
        const auto token_in = key.substr(0, sep);
        const auto token_out = key.substr(sep + 1);
        std::uint64_t amount_out = 0;
        if (!tokens_->quote_exact_in(token_in,
                                     token_out,
                                     static_cast<std::uint64_t>(value),
                                     amount_out,
                                     error)) {
            return false;
        }
        out = std::to_string(amount_out);
        return true;
    }

    error = "unsupported method";
    return false;
}

std::string ContractEngine::dump_state() const {
    std::ostringstream oss;
    for (const auto& [id, c] : contracts_) {
        oss << "C|" << id << '|' << c.owner << '|' << c.code << '\n';
        for (const auto& [k, v] : c.kv) {
            oss << "K|" << id << '|' << k << '|' << v << '\n';
        }
    }
    return oss.str();
}

bool ContractEngine::load_state(const std::string& state, std::string& error) {
    contracts_.clear();

    std::istringstream iss(state);
    for (std::string line; std::getline(iss, line);) {
        if (line.empty()) {
            continue;
        }

        std::istringstream ls(line);
        std::string tag;
        std::getline(ls, tag, '|');

        if (tag == "C") {
            std::string id;
            std::string owner;
            std::string code;
            std::getline(ls, id, '|');
            std::getline(ls, owner, '|');
            std::getline(ls, code);
            if (id.empty()) {
                error = "invalid contract state line";
                return false;
            }
            contracts_[id] = Contract{owner, code, {}};
        } else if (tag == "K") {
            std::string id;
            std::string key;
            std::string value_str;
            std::getline(ls, id, '|');
            std::getline(ls, key, '|');
            std::getline(ls, value_str);
            if (id.empty()) {
                error = "invalid contract kv line";
                return false;
            }
            contracts_[id].kv[key] = std::stoll(value_str);
        }
    }

    return true;
}

} // namespace addition
