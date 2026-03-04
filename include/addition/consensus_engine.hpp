#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace addition {

class ConsensusEngine {
public:
    void submit_vote(const std::string& peer,
                     std::uint64_t height,
                     const std::string& block_hash);

    std::size_t vote_count(std::uint64_t height, const std::string& block_hash) const;
    bool has_quorum(std::uint64_t height,
                    const std::string& block_hash,
                    std::size_t total_peers) const;

private:
    // height -> block_hash -> set(peer)
    std::unordered_map<std::uint64_t,
                       std::unordered_map<std::string, std::unordered_set<std::string>>> votes_;
};

} // namespace addition
