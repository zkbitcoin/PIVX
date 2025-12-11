// external/mn.cpp
#include "external/mn.h"

#include "masternodeman.h"
#include "masternode.h"
#include "netbase.h"
#include "timedata.h"
#include "util/system.h"

#include <string>
#include <sstream>
#include <cstdio>

//
// Debug macro
//
#define MNDBG(x) printf("[MNDBG] %s\n", x)

//
// Hold last JSON string
//
static std::string g_last;
static const char* ret(const std::string& s) {
    g_last = s;
    return g_last.c_str();
}

//
// Only inject once
//
static bool g_injected = false;

//
// Convert CMasternode → JSON
//
static std::string mn_to_json(const CMasternode& mn)
{
    MNDBG("mn_to_json: ENTER");

    std::ostringstream o;
    o << "{";
    o << "\"vin\":\"" << mn.vin.ToString() << "\",";
    o << "\"addr\":\"" << mn.addr.ToString() << "\",";
    o << "\"protocol_version\":" << mn.protocolVersion << ",";
    o << "\"sig_time\":" << mn.sigTime << ",";
    o << "\"status\":\"" << mn.Status() << "\",";
    o << "\"last_ping\":{";
    o << "\"blockhash\":\"" << mn.lastPing.blockHash.ToString() << "\",";
    o << "\"sigtime\":" << mn.lastPing.sigTime;
    o << "}";
    o << "}";

    MNDBG("mn_to_json: EXIT");
    return o.str();
}

//
// SAFELY inject a real legacy-style MN into mnodeman
//
static void inject_fake_masternode()
{
    MNDBG("inject_fake_masternode: ENTER");

    if (g_injected) {
        MNDBG("inject_fake_masternode: already injected");
        return;
    }

    g_injected = true;

    //
    // 1. Create fake collateral
    //
    MNDBG("creating fake COutPoint");
    COutPoint collateral(uint256S("01"), 0);

    //
    // 2. Construct CMasternode object
    //
    MNDBG("constructing CMasternode");
    CMasternode mn;
    mn.vin = CTxIn(collateral);
    mn.protocolVersion = PROTOCOL_VERSION;
    mn.sigTime = GetAdjustedTime();

    //
    // 3. Address (legacy masternodes require service port)
    //
    MNDBG("creating service via LookupNumeric()");
    CService service = LookupNumeric("127.0.0.1", 51472);
    mn.addr = service;

    MNDBG("service OK");

    //
    // 4. Construct a valid ping
    //
    MNDBG("constructing ping object");
    CMasternodePing ping(
        mn.vin,
        uint256S("02"),       // fake blockhash
        GetAdjustedTime()
    );

    MNDBG("setting ping");
    mn.SetLastPing(ping);

    //
    // 5. Actually ADD TO MN MAN
    //
    MNDBG("adding MN via mnodeman.Add()");
    bool ok = mnodeman.Add(mn);

    if (!ok)
        MNDBG("mnodeman.Add() returned FALSE");
    else
        MNDBG("mnodeman.Add() succeeded");

    MNDBG("inject_fake_masternode: EXIT");
}

//
// Public external step API
//
extern "C"
const char* pivx_external_mn_step(void)
{
    MNDBG("pivx_external_mn_step: ENTER");

    //
    // Inject MN if needed
    //
    inject_fake_masternode();

    //
    // Copy the current list (safe accessor)
    //
    MNDBG("copying MN list...");
    std::map<COutPoint, MasternodeRef> list = mnodeman.Copy();

    if (list.empty()) {
        MNDBG("MN list is EMPTY");
        return ret("{\"error\":\"no masternodes present\"}");
    }

    MNDBG("MN list non-empty, extracting first entry");

    const auto& pair = *list.begin();
    const MasternodeRef& ref = pair.second;

    if (!ref) {
        MNDBG("ERROR: MasternodeRef is null");
        return ret("{\"error\":\"null masternode ref\"}");
    }

    MNDBG("building JSON...");

    const CMasternode& mn = *ref;
    std::string json = mn_to_json(mn);

    MNDBG("pivx_external_mn_step: EXIT");
    return ret(json);
}
