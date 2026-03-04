#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace addition {

struct PrivateNote {
    std::string owner;
    std::uint64_t amount{0};
    std::string blinding;
    std::string commitment;
    std::string nullifier;
    bool spent{false};
};

class PrivacyPool {
public:
    bool set_verifier_command(const std::string& command, std::string& error);
    void set_strict_zk_mode(bool enabled);
    bool strict_zk_mode() const;

    std::string mint(const std::string& owner, std::uint64_t amount, std::string& error);
    std::string mint_zk(const std::string& owner,
                        std::uint64_t amount,
                        const std::string& commitment,
                        const std::string& nullifier,
                        const std::string& proof_hex,
                        const std::string& verification_key_hex,
                        std::string& error);
    bool spend(const std::string& owner,
               const std::string& note_id,
               const std::string& recipient,
               std::uint64_t amount,
               std::string& new_note_id,
               std::string& error);
    bool spend_zk(const std::string& owner,
                  const std::string& note_id,
                  const std::string& recipient,
                  std::uint64_t amount,
                  const std::string& nullifier,
                  const std::string& proof_hex,
                  const std::string& verification_key_hex,
                  std::string& new_note_id,
                  std::string& error);

    std::uint64_t private_balance(const std::string& owner) const;
    bool verifier_configured() const;
    std::size_t note_count() const;
    std::size_t used_nullifier_count() const;

    std::string dump_state() const;
    bool load_state(const std::string& state, std::string& error);

private:
    bool verify_external_proof(const std::string& public_input,
                               const std::string& proof_hex,
                               const std::string& verification_key_hex,
                               std::string& error) const;

    std::string verifier_command_;
    bool strict_zk_mode_{true};
    std::unordered_map<std::string, PrivateNote> notes_;
    std::unordered_set<std::string> used_nullifiers_;
};

} // namespace addition
