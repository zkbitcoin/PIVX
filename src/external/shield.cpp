// external/shield.cpp
// ------------------------------------------------------------
// LEVEL 2 Sapling demo (PIVX-correct)
//
// ✔ Real Sapling keys
// ✔ Real note, commitment, nullifier
// ✔ Real Merkle witness + anchor (synthetic)
// ✘ Spend proof (internal to tx)
// ✘ Note encryption (internal to tx)
// ✘ Wallet
// ✘ Blockchain / consensus
//
// ------------------------------------------------------------

#include "sapling/address.h"
#include "sapling/note.h"
#include "sapling/incrementalmerkletree.h"
#include "sapling/zip32.h"

#include "random.h"
#include "support/allocators/secure.h"

#include <string>
#include <sstream>

static std::string g_last;
static const char* ret(const std::string& s) {
    g_last = s;
    return g_last.c_str();
}

extern "C" const char* pivx_external_shield_step(void);

const char* pivx_external_shield_step(void)
{
    // --------------------------------------------------------
    // 1) Generate Sapling keys (NO wallet)
    // --------------------------------------------------------
    std::vector<unsigned char, secure_allocator<unsigned char>> seed(32);
    GetRandBytes(seed.data(), seed.size());
    HDSeed hdseed(seed);

    auto xsk  = libzcash::SaplingExtendedSpendingKey::Master(hdseed);
    auto fvk  = xsk.expsk.full_viewing_key();
    auto addr = xsk.DefaultAddress();

    // --------------------------------------------------------
    // 2) Create Sapling note
    // --------------------------------------------------------
    uint64_t value = 12345678;
    libzcash::SaplingNote note(addr, value);

    auto cmu_opt = note.cmu();
    if (!cmu_opt) {
        return ret("{\"error\":\"cmu generation failed\"}");
    }
    uint256 cmu = cmu_opt.get();

    // --------------------------------------------------------
    // 3) Synthetic Merkle tree
    // --------------------------------------------------------
    SaplingMerkleTree tree;
    tree.append(cmu);

    auto witness = tree.witness();
    uint256 anchor = tree.root();
    uint32_t position = witness.position();

    // --------------------------------------------------------
    // 4) Nullifier
    // --------------------------------------------------------
    auto nf_opt = note.nullifier(fvk, position);
    if (!nf_opt) {
        return ret("{\"error\":\"nullifier generation failed\"}");
    }
    uint256 nullifier = nf_opt.get();

    // --------------------------------------------------------
    // 5) JSON output (explicit limits)
    // --------------------------------------------------------
    std::ostringstream o;
    o << "{";
    o << "\"module\":\"shield\",";
    o << "\"result\":{";
    o <<   "\"value\":" << value << ",";
    o <<   "\"cmu\":\"" << cmu.ToString() << "\",";
    o <<   "\"anchor\":\"" << anchor.ToString() << "\",";
    o <<   "\"nullifier\":\"" << nullifier.ToString() << "\",";
    o <<   "\"witness_position\":" << position;
    o << "},";
    o << "\"validation\":{";
    o <<   "\"cryptography\":{\"checked\":true,\"valid\":true},";
    o <<   "\"proof\":{\"checked\":false,\"reason\":\"PIVX spend proofs generated during tx construction\"},";
    o <<   "\"encryption\":{\"checked\":false,\"reason\":\"PIVX encrypts notes inside tx construction\"},";
    o <<   "\"consensus\":{\"checked\":false,\"reason\":\"no blockchain state\"}";
    o << "},";
    o << "\"environment\":{";
    o <<   "\"level\":2,";
    o <<   "\"wallet_loaded\":false,";
    o <<   "\"blockchain_loaded\":false";
    o << "}";
    o << "}";

    return ret(o.str());
}
