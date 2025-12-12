// external/mn.cpp
// ============================================================================
// Stateless Masternode Demo (UIX)
//
// Each call:
//  • Resets masternode state (stateless demo)
//  • Generates fresh ECDSA keys
//  • Creates a synthetic masternode
//  • Signs a real masternode ping
//  • Injects it into mnodeman (in-memory only)
//  • Returns a JSON snapshot for UI
//
// ⚠️ No wallet, no real collateral, no economic validation
// ============================================================================

#include "external/mn.h"
#include "external/environment.h"
#include "external/logger.h"
#include "external/flow.h"

#include "masternodeman.h"
#include "masternode.h"
#include "netbase.h"
#include "timedata.h"
#include "util/system.h"
#include "chain.h"
#include "validation.h"
#include "key.h"
#include "key_io.h"
#include "base58.h"

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
// Stateless demo → clear MN list every run
// ----------------------------------------------------------------------------
static void reset_demo_state()
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_RESET",
        "Reset masternode state",
        "Clear in-memory masternode manager for stateless demo run",
        "external/mn.cpp",
        "reset_demo_state"
    });

    LOG_INFO("MN", "Resetting masternode list (stateless demo run)");
    mnodeman.Clear();
}

// ----------------------------------------------------------------------------
// Generate real ECDSA keys
// ----------------------------------------------------------------------------
static void generate_keys(
    CKey& mnKey,
    CPubKey& mnPubKey,
    CKey& collateralKey,
    CPubKey& collateralPubKey
)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_KEYGEN",
        "Generate ECDSA keys",
        "Generate fresh secp256k1 key pairs for masternode and collateral",
        "key.h",
        "CKey::MakeNewKey"
    });

    LOG_INFO("MN", "Generating fresh masternode and collateral key pairs");

    mnKey.MakeNewKey(true);
    mnPubKey = mnKey.GetPubKey();

    collateralKey.MakeNewKey(true);
    collateralPubKey = collateralKey.GetPubKey();
}

// ----------------------------------------------------------------------------
// Create, sign, and add masternode
// ----------------------------------------------------------------------------
static CMasternode create_and_add_mn(
    const CKey& mnKey,
    const CPubKey& mnPubKey
)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_CREATE",
        "Create masternode",
        "Construct synthetic masternode with demo collateral",
        "masternode.h",
        "CMasternode"
    });

    LOG_INFO("MN", "Creating synthetic masternode");

    // Synthetic collateral (NOT economically valid)
    COutPoint collateral(uint256S("01"), 0);

    CMasternode mn;
    mn.vin = CTxIn(collateral);
    mn.protocolVersion = PROTOCOL_VERSION;
    mn.sigTime = GetAdjustedTime();
    mn.addr = LookupNumeric("127.0.0.1", 51472);

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_PING_SIGN",
        "Sign masternode ping",
        "Sign masternode ping using ECDSA over secp256k1",
        "masternode.h",
        "CMasternodePing::Sign"
    });

    CMasternodePing ping(
        mn.vin,
        chainActive.Tip()->GetBlockHash(),
        GetAdjustedTime()
    );

    ping.Sign(mnKey, mnPubKey.GetID());
    mn.SetLastPing(ping);

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_REGISTER",
        "Register masternode",
        "Inject masternode into in-memory masternode manager",
        "masternodeman.h",
        "CMasternodeMan::Add"
    });

    mnodeman.Add(mn);

    return mn;
}

// ----------------------------------------------------------------------------
// Build JSON snapshot
// ----------------------------------------------------------------------------
static std::string mn_to_json(
    const CMasternode& mn,
    const CPubKey& mnPubKey,
    const CPubKey& collateralPubKey,
    const CKey& mnKey,
    const CKey& collateralKey
)
{
    std::ostringstream o;
    o << "{";
    o << "\"vin\":\"" << mn.vin.ToString() << "\",";
    o << "\"addr\":\"" << mn.addr.ToString() << "\",";
    o << "\"protocol_version\":" << mn.protocolVersion << ",";
    o << "\"sig_time\":" << mn.sigTime << ",";
    o << "\"status\":\"" << mn.Status() << "\",";
    o << "\"keys\":{";
    o << "\"masternode_pubkey\":\"" << HexStr(mnPubKey) << "\",";
    o << "\"collateral_pubkey\":\"" << HexStr(collateralPubKey) << "\"";
    o << "},";
    o << "\"demo_private_keys\":{";
    o << "\"masternode_privkey\":\"" << KeyIO::EncodeSecret(mnKey) << "\",";
    o << "\"collateral_privkey\":\"" << KeyIO::EncodeSecret(collateralKey) << "\"";
    o << "}";
    o << "}";
    return o.str();
}

// ----------------------------------------------------------------------------
// Public UIX entry point
// ----------------------------------------------------------------------------
extern "C"
const char* pivx_external_mn_step(void)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_ENTRY",
        "Start masternode demo",
        "Entry point for masternode demo execution",
        "external/mn.cpp",
        "pivx_external_mn_step"
    });

    LOG_INFO("MN", "Starting masternode demo step");

    init_environment();
    reset_demo_state();

    CKey    mnKey, collateralKey;
    CPubKey mnPubKey, collateralPubKey;

    generate_keys(mnKey, mnPubKey, collateralKey, collateralPubKey);
    CMasternode mn = create_and_add_mn(mnKey, mnPubKey);

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_EXIT",
        "Return snapshot",
        "Return masternode JSON snapshot to UIX",
        "external/mn.cpp",
        "pivx_external_mn_step"
    });

    LOG_INFO("MN", "Masternode demo step complete");

    return ret(mn_to_json(
        mn,
        mnPubKey,
        collateralPubKey,
        mnKey,
        collateralKey
    ));
}
