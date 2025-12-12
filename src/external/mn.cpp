// external/mn.cpp
// ============================================================================
// Stateless Masternode Demo (UIX)
// Each call generates a NEW masternode with fresh keys,
// signs a real ping, injects it into mnodeman,
// and returns a JSON snapshot.
// ============================================================================

#include "external/mn.h"
#include "external/environment.h"

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
#include <cstdio>

#define MNDBG(x) printf("[MNDBG] %s\n", x)

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
    MNDBG("reset_demo_state");
    mnodeman.Clear();
}

// ----------------------------------------------------------------------------
// Generate real ECDSA keys (ECC already initialized)
// ----------------------------------------------------------------------------
static void generate_keys(
    CKey& mnKey,
    CPubKey& mnPubKey,
    CKey& collateralKey,
    CPubKey& collateralPubKey
)
{
    MNDBG("generate_keys");

    mnKey.MakeNewKey(true);
    mnPubKey = mnKey.GetPubKey();

    collateralKey.MakeNewKey(true);
    collateralPubKey = collateralKey.GetPubKey();
}

// ----------------------------------------------------------------------------
// Create, sign, and add masternode (demo economics)
// ----------------------------------------------------------------------------
static CMasternode create_and_add_mn(
    const CKey& mnKey,
    const CPubKey& mnPubKey
)
{
    MNDBG("create_and_add_mn");

    COutPoint collateral(uint256S("01"), 0);

    CMasternode mn;
    mn.vin = CTxIn(collateral);
    mn.protocolVersion = PROTOCOL_VERSION;
    mn.sigTime = GetAdjustedTime();
    mn.addr = LookupNumeric("127.0.0.1", 51472);

    CMasternodePing ping(
        mn.vin,
        chainActive.Tip()->GetBlockHash(),
        GetAdjustedTime()
    );

    ping.Sign(mnKey, mnPubKey.GetID());
    mn.SetLastPing(ping);

    mnodeman.Add(mn);
    return mn;
}

// ----------------------------------------------------------------------------
// Build UIX JSON snapshot
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
    MNDBG("pivx_external_mn_step: ENTER");

    init_environment();
    reset_demo_state();

    CKey mnKey, collateralKey;
    CPubKey mnPubKey, collateralPubKey;

    generate_keys(mnKey, mnPubKey, collateralKey, collateralPubKey);
    CMasternode mn = create_and_add_mn(mnKey, mnPubKey);

    MNDBG("pivx_external_mn_step: EXIT");
    return ret(mn_to_json(
        mn,
        mnPubKey,
        collateralPubKey,
        mnKey,
        collateralKey
    ));
}
