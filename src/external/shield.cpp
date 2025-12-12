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
// ============================================================================

#include "external/logger.h"
#include "external/flow.h"

#include "sapling/address.h"
#include "sapling/note.h"
#include "sapling/incrementalmerkletree.h"
#include "sapling/zip32.h"

#include "random.h"
#include "support/allocators/secure.h"

#include <string>
#include <sstream>
#include <vector>

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
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_ENTRY",
        "Start Sapling demo",
        "Entry point for Sapling Level 2 demo execution",
        "external/shield.cpp"
    });

    LOG_INFO("SHIELD", "Starting Sapling Level 2 demo");

    // --------------------------------------------------------
    // 1) Generate Sapling keys (NO wallet)
    // --------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_KEYGEN",
        "Generate Sapling keys",
        "Generate Sapling master spending key and viewing key",
        "sapling/zip32.h"
    });

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

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_NOTE_CREATE",
        "Create Sapling note",
        "Create Sapling note with value and recipient address",
        "sapling/note.h"
    });

    LOG_INFO(
        "SHIELD",
        "Creating Sapling note with value " + std::to_string(value)
    );

    libzcash::SaplingNote note(addr, value);

    // --------------------------------------------------------
    // 3) Commitment (cmu)
    // --------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_COMMITMENT",
        "Compute note commitment",
        "Compute Sapling note commitment (cmu)",
        "sapling/note.h"
    });

    auto cmu_opt = note.cmu();
    if (!cmu_opt) {
        LOG_ERROR("SHIELD", "Failed to compute note commitment (cmu)");
        return ret("{\"error\":\"cmu generation failed\"}");
    }

    uint256 cmu = cmu_opt.get();

    LOG_INFO("SHIELD", "Note commitment (cmu) computed");

    // --------------------------------------------------------
    // 4) Synthetic Merkle tree
    // --------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_MERKLE_TREE",
        "Build Merkle tree",
        "Insert commitment into synthetic Sapling Merkle tree",
        "sapling/incrementalmerkletree.h"
    });

    LOG_INFO("SHIELD", "Building synthetic Sapling Merkle tree");

    SaplingMerkleTree tree;
    tree.append(cmu);

    auto witness   = tree.witness();
    uint256 anchor = tree.root();
    uint32_t position = witness.position();

    LOG_INFO(
        "SHIELD",
        "Merkle witness created at position " + std::to_string(position)
    );

    // --------------------------------------------------------
    // 5) Nullifier
    // --------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_NULLIFIER",
        "Derive nullifier",
        "Derive nullifier from note, viewing key, and position",
        "sapling/note.h"
    });

    LOG_INFO("SHIELD", "Deriving nullifier from note and viewing key");

    auto nf_opt = note.nullifier(fvk, position);
    if (!nf_opt) {
        LOG_ERROR("SHIELD", "Nullifier derivation failed");
        return ret("{\"error\":\"nullifier generation failed\"}");
    }

    uint256 nullifier = nf_opt.get();

    LOG_INFO("SHIELD", "Nullifier derived successfully");

    // --------------------------------------------------------
    // 6) JSON output
    // --------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_RESULT",
        "Assemble result",
        "Assemble Sapling demo result JSON",
        "external/shield.cpp"
    });

    std::ostringstream o;
    o << "{"
      << "\"module\":\"shield\","
      << "\"result\":{"
      << "\"value\":" << value << ","
      << "\"cmu\":\"" << cmu.ToString() << "\","
      << "\"anchor\":\"" << anchor.ToString() << "\","
      << "\"nullifier\":\"" << nullifier.ToString() << "\","
      << "\"witness_position\":" << position
      << "},"
      << "\"validation\":{"
      << "\"cryptography\":{\"checked\":true,\"valid\":true},"
      << "\"proof\":{"
         "\"checked\":false,"
         "\"reason\":\"Spend proofs are generated during transaction construction\""
        "},"
      << "\"encryption\":{"
         "\"checked\":false,"
         "\"reason\":\"Note encryption is performed during transaction construction\""
        "},"
      << "\"consensus\":{"
         "\"checked\":false,"
         "\"reason\":\"No blockchain or consensus state loaded\""
        "}"
      << "},"
      << "\"environment\":{"
      << "\"level\":2,"
      << "\"wallet_loaded\":false,"
      << "\"blockchain_loaded\":false"
      << "}"
      << "}";

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_EXIT",
        "Return result",
        "Return Sapling demo result to UIX",
        "external/shield.cpp"
    });

    LOG_INFO("SHIELD", "Sapling Level 2 demo complete");

    return ret(o.str());
}
