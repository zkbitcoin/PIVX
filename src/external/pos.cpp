// external/pos.cpp  —  DEBUG INSTRUMENTED VERSION
//--------------------------------------------------

#include "external/pos.h"
#include "external/external_api.h"
#include "external/environment.h"

#include "chain.h"
#include "kernel.h"
#include "stakeinput.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "util/system.h"
#include "validation.h"

#include <memory>
#include <string>

#define POSDBG(x) printf("[POSDBG] %s\n", x)

static std::string last_json;

/*******************************************************
 * FAKE STAKE INPUT (minimal CStakeInput subclass)
 *******************************************************/
class CFakeStake : public CStakeInput
{
private:
    CTxOut out;
    COutPoint op;
    const CBlockIndex* idx;

public:
    CFakeStake(const CTxOut& o, const COutPoint& p, const CBlockIndex* i)
        : CStakeInput(i), out(o), op(p), idx(i) {}

    const CBlockIndex* GetIndexFrom() const override { return idx; }

    bool GetTxOutFrom(CTxOut& o) const override { o = out; return true; }

    CAmount GetValue() const override { return out.nValue; }

    bool IsZPIV() const override { return false; }

    CDataStream GetUniqueness() const override {
        CDataStream ss(SER_NETWORK, 0);
        ss << op.n << op.hash;
        return ss;
    }

    bool CreateTxIn(CWallet*, CTxIn&, uint256) const { return false; }
    bool CreateTxOuts(CWallet*, std::vector<CTxOut>&, CAmount) const { return false; }
};


/*******************************************************
 * CreateFakeStake()
 *******************************************************/
static std::unique_ptr<CStakeInput> CreateFakeStake(const CBlockIndex* tip)
{
    POSDBG("CreateFakeStake: A");

    CTxOut fake(5000 * COIN, CScript() << OP_TRUE);
    COutPoint fakeOP(uint256S("01"), 0);

    POSDBG("CreateFakeStake: B (construct CFakeStake)");
    auto ptr = std::make_unique<CFakeStake>(fake, fakeOP, tip);

    POSDBG("CreateFakeStake: C return");
    return ptr;
}

bool GetStakeKernelHash_TEST(uint256& hashRet, const CBlock& block, const CBlockIndex* pindexPrev, CStakeInput* stakeInput)
{
    CStakeKernel stakeKernel(pindexPrev, stakeInput, block.nBits, block.nTime);
    hashRet = stakeKernel.GetHash();
    return true;
}

/*******************************************************
 * PUBLIC API — pivx_external_pos_step()
 *******************************************************/
extern "C" const char* pivx_external_pos_step()
{
    POSDBG("STEP 0: ENTER");

    init_environment();
    POSDBG("STEP 1: environment ready");

    const CBlockIndex* tip = chainActive.Tip();
    if (!tip) {
        last_json = R"({"module":"pos","error":"no tip"})";
        return last_json.c_str();
    }
    POSDBG("STEP 2: got chain tip");

    auto stake = CreateFakeStake(tip);
    if (!stake) {
        last_json = R"({"module":"pos","error":"stake create failed"})";
        return last_json.c_str();
    }
    POSDBG("STEP 4: stake constructed");

    // ---------------------------------------------
    // BUILD COINSTAKE TX
    // ---------------------------------------------
    POSDBG("STEP 5: build coinstake tx");

    CMutableTransaction txStake;
    txStake.nVersion = CTransaction::SAPLING;

    // vin --------------------------------------------------
    POSDBG("STEP 6: add vin");
    txStake.vin.emplace_back(tip->GetBlockHash(), 0);

    // vout[0] EMPTY -----------------------------------------
    POSDBG("STEP 7: add empty vout");
    {
        CTxOut o;
        o.SetEmpty();
        txStake.vout.push_back(o);
    }

    // reward vout -------------------------------------------
    POSDBG("STEP 8: add reward");
    txStake.vout.emplace_back(5000 * COIN, CScript() << OP_TRUE);


    // ---------------------------------------------
    // BUILD BLOCK
    // ---------------------------------------------
    POSDBG("STEP 9: build fake block");

    CBlock block;
    block.nTime = tip->nTime + 60;
    block.nBits = tip->nBits;

    // coinbase ------------------------------------------------
    POSDBG("STEP 10: add coinbase");
    {
        CMutableTransaction cb;
        cb.nVersion = CTransaction::SAPLING;
        cb.vin.emplace_back();
        cb.vout.emplace_back();
        block.vtx.emplace_back(MakeTransactionRef(cb));
    }

    // coinstake ------------------------------------------------
    POSDBG("STEP 11: add coinstake tx");
    block.vtx.emplace_back(MakeTransactionRef(txStake));


    /*******************************************************
     * 12A — PRE-CRASH CHECKS
     *******************************************************/
    POSDBG("STEP 12A: CHECK pointers before Stake()");

    const CBlockIndex* idxFrom = stake->GetIndexFrom();
    printf("[POSDBG] addr(stake->GetIndexFrom) = %p\n", (void*)idxFrom);
    printf("[POSDBG] idxFrom->nHeight=%d  nTime=%d\n", idxFrom->nHeight, idxFrom->nTime);


    /*******************************************************
     * 12 — REAL Stake()
     *******************************************************/
    POSDBG("STEP 12: kernel Stake() call");

    int64_t newTime = block.nTime;
    bool hit = Stake(tip, stake.get(), tip->nBits, newTime);

    POSDBG("STEP 13: Stake() returned");

    block.nTime = newTime;


    /*******************************************************
     * KERNEL HASH OUTPUT
     *******************************************************/
    POSDBG("STEP 14: GetStakeKernelHash_TEST()");

    uint256 kernelHash;
    GetStakeKernelHash_TEST(kernelHash, block, tip, stake.get());   // <-- FIXED HERE


    POSDBG("STEP 15: JSON build");

    last_json = strprintf(
        R"({"module":"pos","hit":%s,"kernel":"%s","time":%d})",
        hit ? "true" : "false",
        kernelHash.ToString(),
        block.nTime
    );

    POSDBG("STEP 16: EXIT OK");
    return last_json.c_str();
}
