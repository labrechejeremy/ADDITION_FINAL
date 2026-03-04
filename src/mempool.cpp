#include "addition/mempool.hpp"

#include <algorithm>

namespace addition {

bool Mempool::submit(const Transaction& tx) {
    std::lock_guard<std::mutex> lk(mu_);
    pending_.push_back(tx);
    return true;
}

std::vector<Transaction> Mempool::fetch_for_block(std::size_t max_count) {
    std::lock_guard<std::mutex> lk(mu_);

    std::sort(pending_.begin(), pending_.end(), [](const Transaction& a, const Transaction& b) {
        if (a.fee != b.fee) {
            return a.fee > b.fee;
        }
        if (a.outputs.size() != b.outputs.size()) {
            return a.outputs.size() < b.outputs.size();
        }
        return a.nonce < b.nonce;
    });

    const auto n = (max_count < pending_.size()) ? max_count : pending_.size();
    std::vector<Transaction> out;
    out.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(pending_[i]);
    }
    pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(n));
    return out;
}

std::vector<Transaction> Mempool::snapshot() const {
    std::lock_guard<std::mutex> lk(mu_);
    return pending_;
}

void Mempool::replace(const std::vector<Transaction>& txs) {
    std::lock_guard<std::mutex> lk(mu_);
    pending_ = txs;
}

std::size_t Mempool::size() const {
    std::lock_guard<std::mutex> lk(mu_);
    return pending_.size();
}

} // namespace addition
