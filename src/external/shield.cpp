// external/shield.cpp
// ============================================================================
// Level 2 Sapling Demo
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

static std::string g_last;
static const char* ret(const std::string& s)
{
    g_last = s;
    return g_last.c_str();
}

extern "C"
const char* pivx_external_shield_step(void)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_ENTRY",
        "Start Sapling demo",
        "Entry point for Sapling Level 2 demo",
        "shield.cpp"
    });

    LOG_INFO("SHIELD", "Starting Sapling demo");

    // 1) Generate Sapling keys
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_KEYGEN",
        "Generate Sapling keys",
        "Master spending key and viewing key",
        "sapling/zip32.h"
    });

    std::vector<unsigned char, secure_allocator<unsigned char>> seed(32);
    GetRandBytes(seed.data(), seed.size());
    HDSeed hdseed(seed);

    auto xsk  = libzcash::SaplingExtendedSpendingKey::Master(hdseed);
    auto fvk  = xsk.expsk.full_viewing_key();
    auto addr = xsk.DefaultAddress();

    LOG_INFO("SHIELD", "Sapling payment address derived");

    // 2) Create note
    uint64_t value = 12345678;

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_NOTE_CREATE",
        "Create Sapling note",
        "Note with value and recipient address",
        "sapling/note.h"
    });

    LOG_INFO("SHIELD", "Creating note with value " + std::to_string(value));

    libzcash::SaplingNote note(addr, value);

    // 3) Commitment
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_COMMITMENT",
        "Compute note commitment",
        "Sapling note commitment (cmu)",
        "sapling/note.h"
    });

    auto cmu_opt = note.cmu();
    if (!cmu_opt) {
        LOG_ERROR("SHIELD", "cmu generation failed");
        return ret("{\"error\":\"cmu generation failed\"}");
    }

    uint256 cmu = cmu_opt.get();
    LOG_INFO("SHIELD", "Note commitment computed");

    // 4) Merkle tree
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_MERKLE_TREE",
        "Build Merkle tree",
        "Insert commitment into synthetic tree",
        "sapling/incrementalmerkletree.h"
    });

    LOG_INFO("SHIELD", "Building Merkle tree");

    SaplingMerkleTree tree;
    tree.append(cmu);

    auto witness = tree.witness();
    uint256 anchor = tree.root();
    uint32_t position = witness.position();

    LOG_INFO("SHIELD", "Witness at position " + std::to_string(position));

    // 5) Nullifier
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_NULLIFIER",
        "Derive nullifier",
        "Nullifier from note, viewing key, position",
        "sapling/note.h"
    });

    LOG_INFO("SHIELD", "Deriving nullifier");

    auto nf_opt = note.nullifier(fvk, position);
    if (!nf_opt) {
        LOG_ERROR("SHIELD", "Nullifier derivation failed");
        return ret("{\"error\":\"nullifier generation failed\"}");
    }

    uint256 nullifier = nf_opt.get();
    LOG_INFO("SHIELD", "Nullifier derived");

    // 6) JSON output
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_RESULT",
        "Assemble result",
        "Sapling demo result JSON",
        "shield.cpp"
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
      << "\"proof\":{\"checked\":false,\"reason\":\"Generated during tx construction\"},"
      << "\"encryption\":{\"checked\":false,\"reason\":\"Performed during tx construction\"},"
      << "\"consensus\":{\"checked\":false,\"reason\":\"No blockchain loaded\"}"
      << "},"
      << "\"environment\":{\"level\":2,\"wallet_loaded\":false,\"blockchain_loaded\":false}"
      << "}";

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHIELD,
        "SHIELD_EXIT",
        "Return result",
        "Return Sapling result to UIX",
        "shield.cpp"
    });

    LOG_INFO("SHIELD", "Sapling demo complete");

    return ret(o.str());
}