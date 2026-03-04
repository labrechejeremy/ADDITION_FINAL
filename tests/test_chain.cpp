#include "addition/chain.hpp"
#include "addition/crypto.hpp"
#include "addition/mempool.hpp"
#include "addition/miner.hpp"

#include <iostream>

int main() {
    addition::Chain chain;
    addition::Mempool mempool;
    addition::Miner miner(chain, mempool);

    auto b1 = miner.mine_next_block("miner1");
    std::string error;
    if (!chain.add_block(b1, error)) {
        std::cerr << "test failed: cannot add b1: " << error << '\n';
        return 1;
    }

    if (chain.balance_of("miner1") != 50) {
        std::cerr << "test failed: miner balance mismatch after b1\n";
        return 1;
    }

    addition::Transaction pay{};
    if (!chain.build_transaction("miner1", "bob", 20, 1, 2, pay, error)) {
        std::cerr << "test failed: cannot build tx: " << error << '\n';
        return 1;
    }
    pay.signer = "miner1";
    {
        addition::Transaction unsigned_pay = pay;
        unsigned_pay.signature.clear();
        const auto msg = addition::hash_transaction(unsigned_pay);
        pay.signature = addition::sign_message("miner1", msg);
    }
    mempool.submit(pay);

    auto b2 = miner.mine_next_block("miner1");
    if (!chain.add_block(b2, error)) {
        std::cerr << "test failed: cannot add b2: " << error << '\n';
        return 1;
    }

    if (chain.balance_of("bob") != 20) {
        std::cerr << "test failed: bob balance mismatch\n";
        return 1;
    }

    if (chain.balance_of("miner1") != 79) {
        std::cerr << "test failed: miner change/reward mismatch\n";
        return 1;
    }

    std::cout << "all tests passed\n";
    return 0;
}
