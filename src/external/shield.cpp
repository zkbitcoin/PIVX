// external/shield.cpp
// ============================================================================
// Level 2 Sapling Demo (UIX)
//
// Demonstrates:
//  • Sapling key generation (NO wallet)
//  • Note creation
//  • Commitment (cmu)
//  • Merkle witness + anchor (synthetic tree)
//  • Nullifier derivation
//
// Does NOT demonstrate:
//  • Spend proofs (generated during tx construction)
//  • Note encryption (performed during tx construction)
//  • Wallet integration
//  • Blockchain / consensus validation
//
// ============================================================================

#include "external/logger.h"

#include "sapling/address.h"
#include "sapling/note.h"
#include "sapling/incrementalmerkletree.h"
#include "sapling/zip32.h"

#include "random.h"
#include "support/allocators/secure.h"

#include <string>
#include <sstream>

// ----------------------------------------------------------------------------
// ABI-safe return buffer
// ----------------------------------------------------------------------------
static std::string g_last;
static const char* ret(const std::string& s)
{
    g_last = s;
    return g_last.c_str();
}

// ----------------------------------------------------------------------------
// PUBLIC UIX ENTRY POINT
// ----------------------------------------------------------------------------
extern "C"
const char* pivx_external_shield_step(void)
{
    LOG_INFO("SHIELD", "Starting Sapling Level 2 demo");

    // --------------------------------------------------------
    // 1) Generate Sapling keys (NO wallet)
    // --------------------------------------------------------
    LOG_INFO("SHIELD", "Generating Sapling master keys (wallet-less)");

    std::vector<unsigned char, secure_allocator<unsigned char>> seed(32);
    GetRandBytes(seed.data(), seed.size());
    HDSeed hdseed(seed);

    auto xsk  = libzcash::SaplingExtendedSpendingKey::Master(hdseed);
    auto fvk  = xsk.expsk.full_viewing_key();
    auto addr = xsk.DefaultAddress();

    LOG_INFO("SHIELD", "Sapling payment address derived successfully");

    // --------------------------------------------------------
    // 2) Create Sapling note
    // --------------------------------------------------------
    uint64_t value = 12345678;

    LOG_INFO(
        "SHIELD",
        "Creating Sapling note with value " + std::to_string(value)
    );

    libzcash::SaplingNote note(addr, value);

    auto cmu_opt = note.cmu();
    if (!cmu_opt) {
        LOG_ERROR("SHIELD", "Failed to compute note commitment (cmu)");
        return ret("{\"error\":\"cmu generation failed\"}");
    }

    uint256 cmu = cmu_opt.get();

    LOG_INFO("SHIELD", "Note commitment (cmu) computed");

    // --------------------------------------------------------
    // 3) Synthetic Merkle tree
    // --------------------------------------------------------
    LOG_INFO("SHIELD", "Building synthetic Sapling Merkle tree");

    SaplingMerkleTree tree;
    tree.append(cmu);

    auto witness  = tree.witness();
    uint256 anchor = tree.root();
    uint32_t position = witness.position();

    LOG_INFO(
        "SHIELD",
        "Merkle witness created at position " + std::to_string(position)
    );

    // --------------------------------------------------------
    // 4) Nullifier
    // --------------------------------------------------------
    LOG_INFO("SHIELD", "Deriving nullifier from note and viewing key");

    auto nf_opt = note.nullifier(fvk, position);
    if (!nf_opt) {
        LOG_ERROR("SHIELD", "Nullifier derivation failed");
        return ret("{\"error\":\"nullifier generation failed\"}");
    }

    uint256 nullifier = nf_opt.get();

    LOG_INFO("SHIELD", "Nullifier derived successfully");

    // --------------------------------------------------------
    // 5) JSON output (explicit validation limits)
    // --------------------------------------------------------
    LOG_INFO(
        "SHIELD",
        "Assembling UIX result (proof, encryption, and consensus intentionally omitted)"
    );

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
    o <<   "\"proof\":{"
            "\"checked\":false,"
            "\"reason\":\"Spend proofs are generated during transaction construction\""
          "},";
    o <<   "\"encryption\":{"
            "\"checked\":false,"
            "\"reason\":\"Note encryption is performed during transaction construction\""
          "},";
    o <<   "\"consensus\":{"
            "\"checked\":false,"
            "\"reason\":\"No blockchain or consensus state loaded\""
          "}";
    o << "},";
    o << "\"environment\":{";
    o <<   "\"level\":2,";
    o <<   "\"wallet_loaded\":false,";
    o <<   "\"blockchain_loaded\":false";
    o << "}";
    o << "}";

    LOG_INFO("SHIELD", "Sapling Level 2 demo complete");

    return ret(o.str());
}
