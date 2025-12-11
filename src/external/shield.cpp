// external/shield.cpp
// Minimal Sapling demonstration for external API (no wallet required)

#include "sapling/note.h"
#include "sapling/address.h"
#include "sapling/incrementalmerkletree.h"
#include "sapling/noteencryption.h"
#include "sapling/zip32.h"
#include "sapling/key_io_sapling.h"   // <-- Correct path for EncodePaymentAddress

#include "key_io.h"
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
    //
    // 1) Generate a valid Sapling master spending key from random seed
    //
    std::vector<unsigned char, secure_allocator<unsigned char>> seed(32);
    GetRandBytes(seed.data(), seed.size());
    HDSeed hdseed(seed);

    auto sk  = libzcash::SaplingExtendedSpendingKey::Master(hdseed);
    auto fvk = sk.expsk.full_viewing_key();
    auto ivk = fvk.in_viewing_key();

    // PIVX → default payment address
    auto addr = sk.DefaultAddress();

    //
    // 2) Create a shielded note
    //
    uint64_t value = 12345678;
    libzcash::SaplingNote note(addr, value);

    auto cm_opt = note.cmu();
    if (!cm_opt) return ret("{\"error\":\"cmu generation failed\"}");
    uint256 cmu = cm_opt.get();

    //
    // 3) Construct a tiny Sapling merkle tree with one commitment
    //
    SaplingMerkleTree tree;
    tree.append(cmu);

    //
    // 4) Extract witness + anchor
    //
    auto witness  = tree.witness();
    uint256 anchor = tree.root();
    uint32_t position = witness.position();  // PIVX has position(), not depth()

    //
    // 5) Compute nullifier
    //
    auto nf_opt = note.nullifier(fvk, position);
    if (!nf_opt) return ret("{\"error\":\"nullifier generation failed\"}");
    uint256 nullifier = nf_opt.get();

    //
    // 6) Encode Sapling payment address (correct PIVX API)
    //
    const std::string encodedAddr = KeyIO::EncodePaymentAddress(addr);

    //
    // 7) Produce clean JSON for UI
    //
    std::ostringstream o;
    o << "{";
    o << "\"address\":\"" << encodedAddr << "\",";
    o << "\"value\":" << value << ",";
    o << "\"cmu\":\"" << cmu.ToString() << "\",";
    o << "\"anchor\":\"" << anchor.ToString() << "\",";
    o << "\"nullifier\":\"" << nullifier.ToString() << "\",";
    o << "\"witness_position\":" << position;
    o << "}";

    return ret(o.str());
}
