#include "addition/p2p.hpp"

namespace addition {

bool PeerNetwork::add_peer(const std::string& endpoint) {
    if (endpoint.empty()) {
        return false;
    }
    if (is_banned(endpoint)) {
        return false;
    }
    score_.try_emplace(endpoint, 0);
    return peers_.insert(endpoint).second;
}

bool PeerNetwork::remove_peer(const std::string& endpoint) {
    return peers_.erase(endpoint) > 0;
}

bool PeerNetwork::has_peer(const std::string& endpoint) const {
    return peers_.find(endpoint) != peers_.end();
}

void PeerNetwork::mark_peer_good(const std::string& endpoint) {
    if (!has_peer(endpoint)) {
        return;
    }
    score_[endpoint] += 1;
}

void PeerNetwork::mark_peer_bad(const std::string& endpoint) {
    if (!has_peer(endpoint)) {
        return;
    }
    score_[endpoint] -= 1;
    if (score_[endpoint] <= -5) {
        banned_.insert(endpoint);
        peers_.erase(endpoint);
    }
}

bool PeerNetwork::is_banned(const std::string& endpoint) const {
    return banned_.find(endpoint) != banned_.end();
}

std::vector<std::string> PeerNetwork::peers() const {
    std::vector<std::string> out;
    out.reserve(peers_.size());
    for (const auto& p : peers_) {
        out.push_back(p);
    }
    return out;
}

std::size_t PeerNetwork::peer_count() const { return peers_.size(); }

} // namespace addition
