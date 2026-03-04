#include "addition/privacy.hpp"

#include "addition/crypto.hpp"

#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace addition {
namespace {

std::string mk_note_id(const std::string& owner,
                       std::uint64_t amount,
                       const std::string& seed) {
    return to_hex(sha3_512_bytes("note|" + owner + "|" + std::to_string(amount) + "|" + seed));
}

bool is_hex_even(const std::string& s) {
    if (s.empty() || (s.size() % 2) != 0) {
        return false;
    }
    for (char c : s) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

#ifdef _WIN32
FILE* open_pipe_read(const std::string& cmd) {
    return _popen(cmd.c_str(), "r");
}
int close_pipe_read(FILE* f) {
    return _pclose(f);
}
#else
FILE* open_pipe_read(const std::string& cmd) {
    return popen(cmd.c_str(), "r");
}
int close_pipe_read(FILE* f) {
    return pclose(f);
}
#endif

} // namespace

bool PrivacyPool::set_verifier_command(const std::string& command, std::string& error) {
    if (command.empty()) {
        error = "verifier command empty";
        return false;
    }
    verifier_command_ = command;
    return true;
}

void PrivacyPool::set_strict_zk_mode(bool enabled) {
    strict_zk_mode_ = enabled;
}

bool PrivacyPool::strict_zk_mode() const {
    return strict_zk_mode_;
}

bool PrivacyPool::verify_external_proof(const std::string& public_input,
                                        const std::string& proof_hex,
                                        const std::string& verification_key_hex,
                                        std::string& error) const {
    if (verifier_command_.empty()) {
        error = "privacy verifier command not configured";
        return false;
    }

    if (!is_hex_even(proof_hex) || !is_hex_even(verification_key_hex)) {
        error = "proof/vk must be even-length hex";
        return false;
    }

    if (public_input.find('"') != std::string::npos) {
        error = "public input contains invalid quote";
        return false;
    }

    const std::string cmd = verifier_command_ + " \"" + public_input + "\" \"" + proof_hex + "\" \"" +
                            verification_key_hex + "\"";

    FILE* pipe = open_pipe_read(cmd);
    if (pipe == nullptr) {
        error = "failed to execute verifier command";
        return false;
    }

    std::array<char, 512> buf{};
    std::string output;
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
        output.append(buf.data());
        if (output.size() > 4096) {
            break;
        }
    }

    const int rc = close_pipe_read(pipe);
    if (rc != 0) {
        error = "verifier non-zero exit";
        return false;
    }

    if (output.rfind("OK", 0) == 0 || output.rfind("ok", 0) == 0 || output.rfind("true", 0) == 0 ||
        output.rfind("1", 0) == 0) {
        return true;
    }

    error = "verifier rejected proof";
    return false;
}

std::string PrivacyPool::mint(const std::string& owner, std::uint64_t amount, std::string& error) {
    if (strict_zk_mode_) {
        error = "strict_zk_mode enabled: use privacy_mint_zk";
        return {};
    }
    if (owner.empty()) {
        error = "owner empty";
        return {};
    }
    if (amount == 0) {
        error = "amount must be > 0";
        return {};
    }

    const auto blind = to_hex(sha3_512_bytes("blind|" + owner + "|" + std::to_string(notes_.size())));
    const auto note_id = mk_note_id(owner, amount, blind);
    const auto commitment = to_hex(sha3_512_bytes("cm|" + blind + "|" + std::to_string(amount)));
    const auto nullifier = to_hex(sha3_512_bytes("nf|" + note_id));

    notes_[note_id] = PrivateNote{owner, amount, blind, commitment, nullifier, false};
    return note_id;
}

std::string PrivacyPool::mint_zk(const std::string& owner,
                                 std::uint64_t amount,
                                 const std::string& commitment,
                                 const std::string& nullifier,
                                 const std::string& proof_hex,
                                 const std::string& verification_key_hex,
                                 std::string& error) {
    if (owner.empty() || amount == 0) {
        error = "invalid mint_zk params";
        return {};
    }
    if (!is_hex_even(commitment) || !is_hex_even(nullifier)) {
        error = "commitment/nullifier must be even-length hex";
        return {};
    }
    if (used_nullifiers_.count(nullifier)) {
        error = "nullifier already used";
        return {};
    }

    const std::string public_input = "mint|" + owner + "|" + std::to_string(amount) + "|" + commitment + "|" +
                                     nullifier;
    if (!verify_external_proof(public_input, proof_hex, verification_key_hex, error)) {
        return {};
    }

    const auto note_id = mk_note_id(owner, amount, commitment);
    if (notes_.count(note_id)) {
        error = "note already exists";
        return {};
    }

    notes_[note_id] = PrivateNote{owner, amount, "zk", commitment, nullifier, false};
    return note_id;
}

bool PrivacyPool::spend(const std::string& owner,
                        const std::string& note_id,
                        const std::string& recipient,
                        std::uint64_t amount,
                        std::string& new_note_id,
                        std::string& error) {
    if (strict_zk_mode_) {
        error = "strict_zk_mode enabled: use privacy_spend_zk";
        return false;
    }
    if (owner.empty() || recipient.empty() || note_id.empty()) {
        error = "invalid spend params";
        return false;
    }
    if (amount == 0) {
        error = "amount must be > 0";
        return false;
    }

    auto it = notes_.find(note_id);
    if (it == notes_.end()) {
        error = "note not found";
        return false;
    }
    auto& note = it->second;
    if (note.spent) {
        error = "note already spent";
        return false;
    }
    if (note.owner != owner) {
        error = "owner mismatch";
        return false;
    }
    if (used_nullifiers_.count(note.nullifier)) {
        error = "nullifier already used";
        return false;
    }
    if (note.amount < amount) {
        error = "insufficient note amount";
        return false;
    }

    note.spent = true;
    used_nullifiers_.insert(note.nullifier);

    std::string mint_error;
    new_note_id = mint(recipient, amount, mint_error);
    if (!mint_error.empty()) {
        error = mint_error;
        return false;
    }

    const auto change = note.amount - amount;
    if (change > 0) {
        std::string change_note;
        change_note = mint(owner, change, mint_error);
        if (!mint_error.empty() || change_note.empty()) {
            error = "failed to mint change note";
            return false;
        }
    }

    return true;
}

bool PrivacyPool::spend_zk(const std::string& owner,
                           const std::string& note_id,
                           const std::string& recipient,
                           std::uint64_t amount,
                           const std::string& nullifier,
                           const std::string& proof_hex,
                           const std::string& verification_key_hex,
                           std::string& new_note_id,
                           std::string& error) {
    if (owner.empty() || recipient.empty() || note_id.empty() || amount == 0) {
        error = "invalid spend_zk params";
        return false;
    }

    auto it = notes_.find(note_id);
    if (it == notes_.end()) {
        error = "note not found";
        return false;
    }
    auto& note = it->second;
    if (note.spent) {
        error = "note already spent";
        return false;
    }
    if (note.owner != owner) {
        error = "owner mismatch";
        return false;
    }
    if (note.amount < amount) {
        error = "insufficient note amount";
        return false;
    }
    if (note.nullifier != nullifier) {
        error = "nullifier mismatch";
        return false;
    }
    if (used_nullifiers_.count(nullifier)) {
        error = "nullifier already used";
        return false;
    }

    const std::string public_input = "spend|" + owner + "|" + note_id + "|" + recipient + "|" +
                                     std::to_string(amount) + "|" + nullifier;
    if (!verify_external_proof(public_input, proof_hex, verification_key_hex, error)) {
        return false;
    }

    note.spent = true;
    used_nullifiers_.insert(nullifier);

    std::string mint_error;
    new_note_id = mint(recipient, amount, mint_error);
    if (!mint_error.empty()) {
        error = mint_error;
        return false;
    }

    const auto change = note.amount - amount;
    if (change > 0) {
        std::string change_note;
        change_note = mint(owner, change, mint_error);
        if (!mint_error.empty() || change_note.empty()) {
            error = "failed to mint change note";
            return false;
        }
    }

    return true;
}

std::uint64_t PrivacyPool::private_balance(const std::string& owner) const {
    std::uint64_t total = 0;
    for (const auto& [_, note] : notes_) {
        if (!note.spent && note.owner == owner) {
            total += note.amount;
        }
    }
    return total;
}

bool PrivacyPool::verifier_configured() const {
    return !verifier_command_.empty();
}

std::size_t PrivacyPool::note_count() const {
    return notes_.size();
}

std::size_t PrivacyPool::used_nullifier_count() const {
    return used_nullifiers_.size();
}

std::string PrivacyPool::dump_state() const {
    std::ostringstream oss;
    oss << "M|" << (strict_zk_mode_ ? 1 : 0) << '\n';
    if (!verifier_command_.empty()) {
        oss << "V|" << verifier_command_ << '\n';
    }
    for (const auto& [id, n] : notes_) {
        oss << "N|" << id << '|' << n.owner << '|' << n.amount << '|' << n.blinding << '|' << n.commitment
            << '|' << n.nullifier << '|' << (n.spent ? 1 : 0) << '\n';
    }
    for (const auto& nf : used_nullifiers_) {
        oss << "U|" << nf << '\n';
    }
    return oss.str();
}

bool PrivacyPool::load_state(const std::string& state, std::string& error) {
    notes_.clear();
    used_nullifiers_.clear();
    verifier_command_.clear();
    strict_zk_mode_ = true;

    std::istringstream iss(state);
    for (std::string line; std::getline(iss, line);) {
        if (line.empty()) {
            continue;
        }
        if (line.size() < 2 || line[1] != '|') {
            continue;
        }
        const char tag = line[0];
        std::istringstream ls(line.substr(2));

        if (tag == 'V') {
            std::string cmd;
            std::getline(ls, cmd);
            verifier_command_ = cmd;
        } else if (tag == 'M') {
            std::string v;
            std::getline(ls, v);
            strict_zk_mode_ = (v == "1" || v == "true");
        } else if (tag == 'N') {
            std::string id, owner, amt, blind, cm, nf, spent;
            std::getline(ls, id, '|');
            std::getline(ls, owner, '|');
            std::getline(ls, amt, '|');
            std::getline(ls, blind, '|');
            std::getline(ls, cm, '|');
            std::getline(ls, nf, '|');
            std::getline(ls, spent);
            if (id.empty()) {
                error = "invalid note line";
                return false;
            }
            notes_[id] = PrivateNote{owner,
                                     static_cast<std::uint64_t>(std::stoull(amt)),
                                     blind,
                                     cm,
                                     nf,
                                     spent == "1"};
        } else if (tag == 'U') {
            std::string nf;
            std::getline(ls, nf);
            if (!nf.empty()) {
                used_nullifiers_.insert(nf);
            }
        }
    }

    return true;
}

} // namespace addition
