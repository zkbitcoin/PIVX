// external/environment.cpp
// ============================================================================
// Minimal mock environment for UIX demos
// - Initializes BOTH ECC contexts (VERIFY + SIGN)
// - Sets chain params (REGTEST)
// - Creates fake chainActive tip
// - Sets up EvoDB + deterministicMNManager
// ============================================================================

#include "environment.h"

#include "chainparams.h"
#include "chain.h"
#include "timedata.h"
#include "streams.h"
#include "spork.h"
#include "evo/evodb.h"
#include "evo/deterministicmns.h"
#include "validation.h"

#include "pubkey.h"   // ECCVerifyHandle
#include "key.h"      // ECC_Start / ECC_Stop

#include <memory>
#include <cstdio>

#define ENVDBG(x) printf("[ENVDBG] %s\n", x)

// ============================================================================
// ECC (VERIFY context must be static lifetime)
// ============================================================================
static ECCVerifyHandle g_verify_handle;

// ============================================================================
// GLOBALS
// ============================================================================
static bool g_env_ready = false;

static std::unique_ptr<CEvoDB> g_evoDb;
extern std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

static CBlockIndex* g_genesis = nullptr;

// ============================================================================
// MOCK BLOCKCHAIN
// ============================================================================
static void mock_blockindex()
{
    ENVDBG("mock_blockindex: enter");
    if (g_genesis) return;

    // ------------------------------------------------------------
    // Fake previous block
    // ------------------------------------------------------------
    CBlockIndex* prev = new CBlockIndex();
    prev->nHeight = -1;
    prev->nTime   = GetTime() - 200;
    prev->nBits   = 0x1f00ffff;
    prev->nChainWork = 1;
    prev->pprev = nullptr;

    prev->SetStakeModifier(0x1111111111111111ULL, true);

    static uint256 prevHash = uint256S(
        "00000000000000000000000000000000000000000000000000000000000000"
    );
    prev->phashBlock = &prevHash;
    mapBlockIndex[prevHash] = prev;

    // ------------------------------------------------------------
    // Fake genesis block
    // ------------------------------------------------------------
    g_genesis = new CBlockIndex();
    g_genesis->nHeight = 0;
    g_genesis->nTime   = GetTime() - 100;
    g_genesis->nBits   = prev->nBits;
    g_genesis->nChainWork = 2;
    g_genesis->pprev = prev;

    g_genesis->SetStakeModifier(0x2222222222222222ULL, true);

    static uint256 ghash = uint256S(
        "00000000000000000000000000000000000000000000000000000000000001"
    );
    g_genesis->phashBlock = &ghash;
    mapBlockIndex[ghash] = g_genesis;

    chainActive.SetTip(g_genesis);
    pindexBestHeader = g_genesis;

    ENVDBG("mock_blockindex: done");
}

// ============================================================================
// SPORK MOCK (no-op)
// ============================================================================
static void mock_sporks()
{
    ENVDBG("mock_sporks (noop)");
}

// ============================================================================
// MN MOCK (no-op)
// ============================================================================
static void mock_mn()
{
    ENVDBG("mock_mn");
}

// ============================================================================
// INIT ENVIRONMENT (IDEMPOTENT, SAFE)
// ============================================================================
void init_environment()
{
    if (g_env_ready) return;

    ENVDBG("init_environment: ENTER");

    // ------------------------------------------------------------
    // 🔑 ECC INITIALIZATION (THIS WAS THE MISSING PIECE)
    // ------------------------------------------------------------
    // VERIFY context: handled by static ECCVerifyHandle above
    // SIGN context: MUST be initialized manually
    ECC_Start();

    SelectParams(CBaseChainParams::REGTEST);
    gArgs.SoftSetArg("-datadir", "/tmp/pivx_mock_env");

    g_evoDb = std::make_unique<CEvoDB>(1 << 20); // 1 MB cache
    deterministicMNManager = std::make_unique<CDeterministicMNManager>(*g_evoDb);

    mock_blockindex();
    mock_sporks();
    mock_mn();

    g_env_ready = true;
    ENVDBG("init_environment: COMPLETE");
}

// ============================================================================
// ADVANCE TIME (OPTIONAL)
// ============================================================================
void advance_time(int64_t sec)
{
    if (g_genesis) g_genesis->nTime += sec;
}

// ============================================================================
// DUMMY WALLET STUB (LINKER ONLY)
// ============================================================================
struct DummyWalletStruct { int unused = 0; };
static DummyWalletStruct dummyWallet;

CWallet& wallet()
{
    // Never used — satisfies linker only
    return *(CWallet*)&dummyWallet;
}
