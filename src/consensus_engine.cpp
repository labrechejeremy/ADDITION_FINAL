#include "addition/consensus_engine.hpp"

namespace addition {

void ConsensusEngine::submit_vote(const std::string& peer,
                                  std::uint64_t height,
                                  const std::string& block_hash) {
    if (peer.empty() || block_hash.empty()) {
        return;
    }
    votes_[height][block_hash].insert(peer);
}

std::size_t ConsensusEngine::vote_count(std::uint64_t height, const std::string& block_hash) const {
    const auto hit = votes_.find(height);
    if (hit == votes_.end()) {
        return 0;
    }
    const auto bit = hit->second.find(block_hash);
    if (bit == hit->second.end()) {
        return 0;
    }
    return bit->second.size();
}

bool ConsensusEngine::has_quorum(std::uint64_t height,
                                 const std::string& block_hash,
                                 std::size_t total_peers) const {
    if (total_peers == 0) {
        return false;
    }
    const auto votes = vote_count(height, block_hash);
    const auto needed = (2 * total_peers) / 3 + 1;
    return votes >= needed;
}

} // namespace addition
