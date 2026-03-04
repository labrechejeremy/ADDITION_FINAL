#include "addition/chain.hpp"
#include "addition/bridge.hpp"
#include "addition/contract_engine.hpp"
#include "addition/decentralized_node.hpp"
#include "addition/consensus_engine.hpp"
#include "addition/mempool.hpp"
#include "addition/miner.hpp"
#include "addition/p2p.hpp"
#include "addition/privacy.hpp"
#include "addition/rpc_server.hpp"
#include "addition/rpc_network_server.hpp"
#include "addition/state_store.hpp"
#include "addition/staking.hpp"
#include "addition/wallet_keys.hpp"
#include "addition/token_engine.hpp"
#include "addition/crypto.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <chrono>

int main() {
    addition::Chain chain;
    addition::Mempool mempool;
    addition::Miner miner(chain, mempool);
    addition::StakingEngine staking;
    addition::TokenEngine tokens;
    addition::ContractEngine contracts(&tokens);
    addition::BridgeEngine bridge;
    addition::PeerNetwork peers;
    addition::ConsensusEngine consensus;
    addition::PrivacyPool privacy;
    addition::WalletKeys node_keys{};
    node_keys.algorithm = "ml-dsa-87";
    {
        const std::string id_path = "data/node_identity.dat";
        if (std::filesystem::exists(id_path)) {
            std::ifstream in(id_path, std::ios::binary);
            std::string line;
            while (std::getline(in, line)) {
                if (line.rfind("PUB|", 0) == 0) {
                    node_keys.public_key = line.substr(4);
                } else if (line.rfind("PRIV|", 0) == 0) {
                    node_keys.private_key = line.substr(5);
                }
            }
        }

        if (node_keys.public_key.empty() || node_keys.private_key.empty()) {
            node_keys = addition::generate_wallet_keys();
            std::filesystem::create_directories("data");
            std::ofstream out(id_path, std::ios::binary | std::ios::trunc);
            out << "PUB|" << node_keys.public_key << '\n';
            out << "PRIV|" << node_keys.private_key << '\n';
        }
    }

    addition::DecentralizedNode node("self",
                                     node_keys.public_key,
                                     node_keys.private_key,
                                     chain,
                                     mempool,
                                     peers,
                                     consensus);
    addition::StateStore store("data");

    {
        std::string report;
        if (!addition::crypto_selftest(report)) {
            std::cerr << "fatal: crypto selftest failed: " << report << '\n';
            return 2;
        }
        std::cout << report << '\n';
    }

    std::string load_error;
    if (!store.load_all(chain, mempool, staking, contracts, tokens, bridge, peers, node, privacy, load_error)) {
        std::cout << "warning: state load failed: " << load_error << '\n';
    } else {
        std::cout << "state loaded from ./data\n";
    }

    addition::RpcNetworkServer p2p_rpc("0.0.0.0", 28545, [&](const std::string& cmd) {
        std::istringstream iss(cmd);
        std::string peer;
        iss >> peer;
        std::string payload;
        std::getline(iss, payload);
        if (!payload.empty() && payload.front() == ' ') {
            payload.erase(payload.begin());
        }

        if (peer.empty() || payload.empty()) {
            return std::string("error: usage <peer> <payload>");
        }

        std::string err;
        if (!node.handle_inbound_message(peer, payload, err)) {
            return std::string("error: ") + err;
        }

        auto outbound = node.pull_outbound_messages();
        if (outbound.empty()) {
            return std::string("ok");
        }
        std::ostringstream out;
        out << "ok:" << outbound.front();
        return out.str();
    });

    addition::RpcServer rpc(chain, mempool, miner, staking, contracts, bridge, tokens, peers, consensus, privacy, node);
    addition::RpcNetworkServer local_rpc("127.0.0.1", 8545, [&](const std::string& cmd) {
        return rpc.handle_command(cmd);
    });
    addition::RpcNetworkServer lan_rpc("0.0.0.0", 18545, [&](const std::string& cmd) {
        return rpc.handle_command(cmd);
    });

    std::string local_error;
    if (!local_rpc.start(local_error)) {
        std::cout << "warning: local RPC failed to start: " << local_error << '\n';
    } else {
        std::cout << "local RPC listening on 127.0.0.1:8545\n";
    }

    std::string lan_error;
    if (!lan_rpc.start(lan_error)) {
        std::cout << "warning: LAN RPC failed to start: " << lan_error << '\n';
    } else {
        std::cout << "LAN RPC listening on 0.0.0.0:18545\n";
    }

    std::string p2p_error;
    if (!p2p_rpc.start(p2p_error)) {
        std::cout << "warning: P2P RPC failed to start: " << p2p_error << '\n';
    } else {
        std::cout << "P2P RPC listening on 0.0.0.0:28545\n";
    }

    std::cout << "ADDITION_FINAL daemon started. Commands: getinfo, fee_info, createwallet, sign_message <privkey_hex> <message_hex_utf8>, getbalance <addr>, getbalance_instant <addr>, sendtx <from_addr> <pubkey_hex> <privkey_hex> <to_addr> <amount> <fee> <nonce>, mine,\n"
                 "monetary_info, crypto_selftest,\n"
                 "stake <addr> <amt>, unstake <addr> <amt>, staked <addr>, stake_reward <amt>, stake_claim <addr>,\n"
                 "stake_claimable <addr>, stake_policy <get|set> [cap_bps admin_addr admin_pubkey admin_sig],\n"
                 "contract_deploy <owner> <code>, contract_call <id> <set|add|get> <key> <value>,\n"
                 "addpeer <ip:port>, delpeer <ip:port>, peers, vote <peer> <height> <hash>, quorum <height> <hash>,\n"
                 "privacy_set_verifier <cmd>, privacy_strict_mode <on|off>, privacy_mint <owner> <amt>, privacy_spend <owner> <note_id> <recipient> <amt>,\n"
                 "privacy_mint_zk <owner> <amt> <commitment> <nullifier> <proof> <vk>,\n"
                 "privacy_spend_zk <owner> <note_id> <recipient> <amt> <nullifier> <proof> <vk>, privacy_balance <owner>, privacy_status,\n"
                 "bridge_register <chain>, bridge_set_attestor <chain> <pubkey> <admin_addr> <admin_pubkey> <admin_sig>, bridge_attestor <chain>,\n"
                 "bridge_lock <chain> <user> <amt>, bridge_mint <chain> <user> <amt>, bridge_mint_attested <chain> <user> <amt> <sig>,\n"
                 "bridge_burn <chain> <user> <amt>, bridge_release <chain> <user> <amt>, bridge_release_attested <chain> <user> <amt> <sig>,\n"
                 "bridge_balance <chain> <user>,\n"
                 "token_create <symbol> <owner> <max_supply> <initial_mint>, token_mint <symbol> <caller> <to> <amount>,\n"
                 "token_create_ex <symbol> <name> <owner> <max_supply> <initial_mint> <decimals> <burnable_0_1> <dev_wallet_or_dash> <dev_allocation>,\n"
                 "token_transfer <symbol> <from> <to> <amount>, token_balance <symbol> <owner>, token_info <symbol>, token_burn <symbol> <from> <amount>,\n"
                 "token_set_policy <symbol> <caller_owner> <treasury_wallet_or_dash> <transfer_fee_bps> <burn_fee_bps> <paused_0_1>,\n"
                 "token_blacklist <symbol> <caller_owner> <wallet> <blocked_0_1>,\n"
                 "token_fee_exempt <symbol> <caller_owner> <wallet> <exempt_0_1>,\n"
                 "token_set_limits <symbol> <caller_owner> <max_tx_amount_or_0> <max_wallet_amount_or_0>,\n"
                 "swap_pool_create <token_a> <token_b> <fee_bps>,\n"
                 "swap_add_liquidity <token_a> <token_b> <provider> <amount_a> <amount_b>,\n"
                 "swap_remove_liquidity <token_a> <token_b> <provider> <lp_amount>,\n"
                 "swap_pool_info <token_a> <token_b>,\n"
                 "swap_quote <token_in> <token_out> <amount_in>,\n"
                 "swap_exact_in <token_in> <token_out> <trader> <amount_in> <min_out>,\n"
                 "swap_quote_route <A>B>C <amount_in>, swap_route_exact_in <A>B>C <trader> <amount_in> <min_out>,\n"
                 "swap_best_route <token_in> <token_out> <amount_in> [max_hops],\n"
                 "swap_best_route_exact_in <token_in> <token_out> <trader> <amount_in> <min_out> <deadline_unix> [max_hops],\n"
                 "swap_best_route_sign_payload <token_in> <token_out> <trader> <amount_in> <min_out> <deadline_unix> [max_hops],\n"
                 "swap_best_route_exact_in_signed <token_in> <token_out> <trader> <amount_in> <min_out> <deadline_unix> <max_hops> <trader_pubkey> <trader_sig>,\n"
                 "nft_mint <collection> <token_id> <owner> <metadata>, nft_transfer <collection> <token_id> <from> <to>, nft_owner <collection> <token_id>,\n"
                 "sendtx_hash <from_addr> <pubkey_hex> <privkey_hex> <to_addr> <amount> <fee> <nonce>, tx_status <tx_hash>,\n"
                 "peer_inbound <peer> <payload>, gossip_flush, sync, node_pubkey,\n"
                 "identity_rotate_propose <new_pub> <new_priv> <grace_sec>, identity_rotate_vote <peer_id>,\n"
                 "identity_rotate_vote_broadcast, identity_rotate_commit, identity_rotate_status, quit\n";

    auto last_sync = std::chrono::steady_clock::now();

    for (std::string line; std::getline(std::cin, line);) {
        const auto now = std::chrono::steady_clock::now();
        if (now - last_sync >= std::chrono::seconds(5)) {
            std::string sync_err;
            node.sync_once(sync_err);
            last_sync = now;
        }

        if (line == "quit" || line == "exit") {
            break;
        }

        if (line.rfind("mine", 0) == 0) {
            std::string reward_address = "miner1";
            {
                std::istringstream ls(line);
                std::string cmd;
                ls >> cmd;
                std::string maybe_address;
                ls >> maybe_address;
                if (!maybe_address.empty()) {
                    reward_address = maybe_address;
                }
            }

            std::string mined_hash;
            std::string error;
            if (!miner.mine_next_block(reward_address, 500, mined_hash, error)) {
                std::cout << "error: " << error << '\n';
            } else {
                std::cout << "mined block " << chain.height() << " reward=" << reward_address
                          << " hash=" << mined_hash << '\n';
            }
            continue;
        }

        std::cout << rpc.handle_command(line) << '\n';
    }

    local_rpc.stop();
    lan_rpc.stop();
    p2p_rpc.stop();

    std::string save_error;
    if (!store.save_all(chain, mempool, staking, contracts, tokens, bridge, peers, node, privacy, save_error)) {
        std::cout << "warning: state save failed: " << save_error << '\n';
    } else {
        std::cout << "state saved to ./data\n";
    }

    return 0;
}
