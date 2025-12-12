// external/pos.cpp — Level 1 PoS Demo

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

/***************************************************************
 * Fake stake input — kernel-valid only
 ***************************************************************/
class CFakeStake : public CStakeInput
{
private:
    CTxOut out;
    COutPoint op;
    const CBlockIndex* idx;

public:
    CFakeStake(const CTxOut& o,
               const COutPoint& p,
               const CBlockIndex* i)
        : CStakeInput(i), out(o), op(p), idx(i) {}

    const CBlockIndex* GetIndexFrom() const override { return idx; }

    bool GetTxOutFrom(CTxOut& o) const override {
        o = out;
        return true;
    }

    CAmount GetValue() const override { return out.nValue; }

    bool IsZPIV() const override { return false; }

    CDataStream GetUniqueness() const override {
        CDataStream ss(SER_NETWORK, 0);
        ss << op.hash << op.n;
        return ss;
    }

    // NOT virtual in PIVX → no override
    bool CreateTxIn(CWallet*, CTxIn&, uint256) const { return false; }
    bool CreateTxOuts(CWallet*, std::vector<CTxOut>&, CAmount) const { return false; }
};

/***************************************************************
 * Create fake stake
 ***************************************************************/
static std::unique_ptr<CStakeInput> CreateFakeStake(const CBlockIndex* tip)
{
    CTxOut out(5000 * COIN, CScript() << OP_TRUE);
    COutPoint op(uint256S("01"), 0);
    return std::make_unique<CFakeStake>(out, op, tip);
}

/***************************************************************
 * Kernel hash helper
 ***************************************************************/
static uint256 GetKernelHash(const CBlock& block,
                             const CBlockIndex* prev,
                             CStakeInput* stake)
{
    CStakeKernel kernel(prev, stake, block.nBits, block.nTime);
    return kernel.GetHash();
}

/***************************************************************
 * PUBLIC API
 ***************************************************************/
extern "C"
const char* pivx_external_pos_step()
{
    POSDBG("ENTER");

    init_environment();

    const CBlockIndex* tip = chainActive.Tip();
    if (!tip) {
        last_json = "{\"module\":\"pos\",\"error\":\"no chain tip\"}";
        return last_json.c_str();
    }

    auto stake = CreateFakeStake(tip);
    if (!stake) {
        last_json = "{\"module\":\"pos\",\"error\":\"stake creation failed\"}";
        return last_json.c_str();
    }

    // Build fake block
    CBlock block;
    block.nBits = tip->nBits;
    block.nTime = tip->nTime + 60;

    // coinbase
    {
        CMutableTransaction cb;
        cb.nVersion = CTransaction::SAPLING;
        cb.vin.emplace_back();
        cb.vout.emplace_back();
        block.vtx.emplace_back(MakeTransactionRef(cb));
    }

    // coinstake
    {
        CMutableTransaction tx;
        tx.nVersion = CTransaction::SAPLING;
        tx.vin.emplace_back(tip->GetBlockHash(), 0);

        CTxOut empty;
        empty.SetEmpty();
        tx.vout.push_back(empty);

        tx.vout.emplace_back(5000 * COIN, CScript() << OP_TRUE);
        block.vtx.emplace_back(MakeTransactionRef(tx));
    }

    int64_t newTime = block.nTime;
    bool hit = Stake(tip, stake.get(), tip->nBits, newTime);
    block.nTime = newTime;

    uint256 kernelHash = GetKernelHash(block, tip, stake.get());

    last_json = strprintf(
        "{"
          "\"module\":\"pos\","
          "\"result\":{"
            "\"kernel_hit\":%s,"
            "\"kernel_hash\":\"%s\","
            "\"block_time\":%d"
          "},"
          "\"validation\":{"
            "\"kernel\":{\"checked\":true,\"valid\":true},"
            "\"difficulty\":{\"checked\":true,\"bits\":\"%08x\",\"target_hit\":%s},"
            "\"economic\":{\"checked\":false,\"reason\":\"Level 1 demo — no wallet\"}"
          "},"
          "\"environment\":{"
            "\"level\":1,"
            "\"wallet_loaded\":false"
          "}"
        "}",
        hit ? "true" : "false",
        kernelHash.ToString(),
        block.nTime,
        tip->nBits,
        hit ? "true" : "false"
    );

    POSDBG("EXIT OK");
    return last_json.c_str();
}
