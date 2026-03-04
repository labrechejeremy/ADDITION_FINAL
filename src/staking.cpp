#include "addition/staking.hpp"

#include <algorithm>

namespace addition {

bool StakingEngine::stake(const std::string& address, std::uint64_t amount, std::string& error) {
    if (address.empty()) {
        error = "address empty";
        return false;
    }
    if (amount == 0) {
        error = "amount must be > 0";
        return false;
    }

    stakes_[address] += amount;
    total_staked_ += amount;
    return true;
}

bool StakingEngine::unstake(const std::string& address, std::uint64_t amount, std::string& error) {
    if (address.empty()) {
        error = "address empty";
        return false;
    }
    if (amount == 0) {
        error = "amount must be > 0";
        return false;
    }

    auto it = stakes_.find(address);
    if (it == stakes_.end() || it->second < amount) {
        error = "insufficient staked balance";
        return false;
    }

    it->second -= amount;
    total_staked_ -= amount;
    return true;
}

std::uint64_t StakingEngine::staked_of(const std::string& address) const {
    auto it = stakes_.find(address);
    return it == stakes_.end() ? 0ULL : it->second;
}

std::uint64_t StakingEngine::total_staked() const { return total_staked_; }

void StakingEngine::distribute_epoch_rewards(std::uint64_t reward_pool) {
    if (reward_pool == 0 || total_staked_ == 0) {
        return;
    }

    const std::uint64_t max_pool = (total_staked_ * reward_cap_bps_) / 10000;
    const std::uint64_t bounded_pool = std::min(reward_pool, max_pool);
    if (bounded_pool == 0) {
        return;
    }

    for (const auto& [addr, staked] : stakes_) {
        const auto reward = (bounded_pool * staked) / total_staked_;
        claimable_[addr] += reward;
    }
}

void StakingEngine::set_reward_cap_bps(std::uint64_t cap_bps) {
    if (cap_bps < 1) {
        reward_cap_bps_ = 1;
        return;
    }
    if (cap_bps > 2000) {
        reward_cap_bps_ = 2000;
        return;
    }
    reward_cap_bps_ = cap_bps;
}

std::uint64_t StakingEngine::reward_cap_bps() const {
    return reward_cap_bps_;
}

std::uint64_t StakingEngine::claimable_of(const std::string& address) const {
    auto it = claimable_.find(address);
    return it == claimable_.end() ? 0ULL : it->second;
}

std::uint64_t StakingEngine::claim(const std::string& address) {
    auto it = claimable_.find(address);
    if (it == claimable_.end()) {
        return 0;
    }
    const auto value = it->second;
    it->second = 0;
    return value;
}

const std::unordered_map<std::string, std::uint64_t>& StakingEngine::stakes_map() const {
    return stakes_;
}

const std::unordered_map<std::string, std::uint64_t>& StakingEngine::claimable_map() const {
    return claimable_;
}

void StakingEngine::replace_state(const std::unordered_map<std::string, std::uint64_t>& stakes,
                                  const std::unordered_map<std::string, std::uint64_t>& claimable,
                                  std::uint64_t total_staked) {
    stakes_ = stakes;
    claimable_ = claimable;
    total_staked_ = total_staked;
}

} // namespace addition
