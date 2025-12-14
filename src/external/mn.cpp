// external/mn.cpp
// ============================================================================
// Stateless Masternode Demo
// ============================================================================

#include "external/mn.h"
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

static std::string g_last;
static const char* ret(const std::string& s)
{
    g_last = s;
    return g_last.c_str();
}

static void generate_keys(CKey& mnKey, CPubKey& mnPubKey,
                          CKey& collateralKey, CPubKey& collateralPubKey)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_KEYGEN",
        "Generate ECDSA keys",
        "Fresh secp256k1 key pairs for masternode and collateral",
        "key.h"
    });

    LOG_INFO("MN", "Generating key pairs");

    mnKey.MakeNewKey(true);
    mnPubKey = mnKey.GetPubKey();

    collateralKey.MakeNewKey(true);
    collateralPubKey = collateralKey.GetPubKey();
}

static CMasternode create_and_add_mn(const CKey& mnKey, const CPubKey& mnPubKey)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_CREATE",
        "Create masternode",
        "Synthetic masternode with demo collateral",
        "masternode.h"
    });

    LOG_INFO("MN", "Creating synthetic masternode");

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
        "ECDSA signature over secp256k1",
        "masternode.h"
    });

    CMasternodePing ping(mn.vin, chainActive.Tip()->GetBlockHash(), GetAdjustedTime());
    ping.Sign(mnKey, mnPubKey.GetID());
    mn.SetLastPing(ping);

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_REGISTER",
        "Register masternode",
        "Inject into in-memory masternode manager",
        "masternodeman.h"
    });

    mnodeman.Add(mn);

    return mn;
}

static std::string mn_to_json(const CMasternode& mn,
                              const CPubKey& mnPubKey,
                              const CPubKey& collateralPubKey,
                              const CKey& mnKey,
                              const CKey& collateralKey)
{
    std::ostringstream o;
    o << "{"
      << "\"module\":\"mn\","
      << "\"result\":{"
      << "\"vin\":\"" << mn.vin.ToString() << "\","
      << "\"addr\":\"" << mn.addr.ToString() << "\","
      << "\"protocol_version\":" << mn.protocolVersion << ","
      << "\"sig_time\":" << mn.sigTime << ","
      << "\"status\":\"" << mn.Status() << "\""
      << "},"
      << "\"validation\":{"
      << "\"network\":{\"checked\":false,\"reason\":\"Stateless demo\"},"
      << "\"economic\":{\"checked\":false,\"reason\":\"No wallet or collateral\"},"
      << "\"governance\":{\"checked\":false,\"reason\":\"Not evaluated in demo\"}"
      << "},"
      << "\"keys\":{"
      << "\"masternode_pubkey\":\"" << HexStr(mnPubKey) << "\","
      << "\"collateral_pubkey\":\"" << HexStr(collateralPubKey) << "\""
      << "},"
      << "\"demo_private_keys\":{"
      << "\"masternode_privkey\":\"" << KeyIO::EncodeSecret(mnKey) << "\","
      << "\"collateral_privkey\":\"" << KeyIO::EncodeSecret(collateralKey) << "\""
      << "},"
      << "\"environment\":{\"level\":1,\"wallet_loaded\":false}"
      << "}";
    return o.str();
}

extern "C"
const char* pivx_external_mn_step(void)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_ENTRY",
        "Start masternode demo",
        "Entry point for masternode demo",
        "mn.cpp"
    });

    LOG_INFO("MN", "Starting masternode demo");

    CKey mnKey, collateralKey;
    CPubKey mnPubKey, collateralPubKey;

    generate_keys(mnKey, mnPubKey, collateralKey, collateralPubKey);
    CMasternode mn = create_and_add_mn(mnKey, mnPubKey);

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::MN,
        "MN_EXIT",
        "Return snapshot",
        "Return JSON to UIX",
        "mn.cpp"
    });

    LOG_INFO("MN", "Masternode demo complete");

    return ret(mn_to_json(mn, mnPubKey, collateralPubKey, mnKey, collateralKey));
}