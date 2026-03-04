#pragma once

#include "addition/chain.hpp"
#include "addition/bridge.hpp"
#include "addition/consensus_engine.hpp"
#include "addition/contract_engine.hpp"
#include "addition/decentralized_node.hpp"
#include "addition/mempool.hpp"
#include "addition/miner.hpp"
#include "addition/p2p.hpp"
#include "addition/privacy.hpp"
#include "addition/staking.hpp"
#include "addition/token_engine.hpp"

#include <string>

namespace addition {

class RpcServer {
public:
    RpcServer(Chain& chain,
              Mempool& mempool,
              Miner& miner,
              StakingEngine& staking,
              ContractEngine& contracts,
              BridgeEngine& bridge,
              TokenEngine& tokens,
              PeerNetwork& peers,
              ConsensusEngine& consensus,
              PrivacyPool& privacy,
              DecentralizedNode& node);

    std::string handle_command(const std::string& line);

private:
    Chain& chain_;
    Mempool& mempool_;
    Miner& miner_;
    StakingEngine& staking_;
    ContractEngine& contracts_;
    BridgeEngine& bridge_;
    TokenEngine& tokens_;
    PeerNetwork& peers_;
    ConsensusEngine& consensus_;
    PrivacyPool& privacy_;
    DecentralizedNode& node_;
};

} // namespace addition
