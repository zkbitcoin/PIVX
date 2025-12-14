// external/pos.cpp
// ============================================================================
// Level 1 Proof-of-Stake Demo
// ============================================================================

#include "external/pos.h"
#include "external/logger.h"
#include "external/flow.h"

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
#include <sstream>

static std::string g_last;

class CFakeStake : public CStakeInput
{
private:
    CTxOut m_out;
    COutPoint m_op;
    const CBlockIndex* m_idx;

public:
    CFakeStake(const CTxOut& o, const COutPoint& p, const CBlockIndex* i)
        : CStakeInput(i), m_out(o), m_op(p), m_idx(i) {}

    const CBlockIndex* GetIndexFrom() const override { return m_idx; }
    bool GetTxOutFrom(CTxOut& o) const override { o = m_out; return true; }
    CAmount GetValue() const override { return m_out.nValue; }
    bool IsZPIV() const override { return false; }

    CDataStream GetUniqueness() const override {
        CDataStream ss(SER_NETWORK, 0);
        ss << m_op.hash << m_op.n;
        return ss;
    }

    bool CreateTxIn(CWallet*, CTxIn&, uint256) const { return false; }
    bool CreateTxOuts(CWallet*, std::vector<CTxOut>&, CAmount) const { return false; }
};

static std::unique_ptr<CStakeInput> CreateFakeStake(const CBlockIndex* tip)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_STAKE_CREATE",
        "Create stake input",
        "Synthetic stake input for kernel hashing",
        "stakeinput.h"
    });

    LOG_INFO("POS", "Creating synthetic stake input");

    CTxOut out(5000 * COIN, CScript() << OP_TRUE);
    COutPoint op(uint256S("01"), 0);

    return std::make_unique<CFakeStake>(out, op, tip);
}

static uint256 GetKernelHash(const CBlock& block, const CBlockIndex* prev, CStakeInput* stake)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_KERNEL_HASH",
        "Compute kernel hash",
        "Stake kernel hash for PoS evaluation",
        "kernel.h"
    });

    CStakeKernel kernel(prev, stake, block.nBits, block.nTime);
    return kernel.GetHash();
}

extern "C"
const char* pivx_external_pos_step()
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_ENTRY",
        "Start PoS demo",
        "Entry point for PoS Level 1 demo",
        "pos.cpp"
    });

    LOG_INFO("POS", "Starting PoS demo");

    const CBlockIndex* tip = chainActive.Tip();
    if (!tip) {
        g_last = "{\"module\":\"pos\",\"error\":\"no chain tip\"}";
        return g_last.c_str();
    }

    auto stake = CreateFakeStake(tip);
    if (!stake) {
        g_last = "{\"module\":\"pos\",\"error\":\"stake creation failed\"}";
        return g_last.c_str();
    }

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_BLOCK_BUILD",
        "Build synthetic block",
        "PoS block with coinbase and coinstake",
        "primitives/block.h"
    });

    CBlock block;
    block.nBits = tip->nBits;
    block.nTime = tip->nTime + 60;

    {
        CMutableTransaction cb;
        cb.nVersion = CTransaction::SAPLING;
        cb.vin.emplace_back();
        cb.vout.emplace_back();
        block.vtx.emplace_back(MakeTransactionRef(cb));
    }

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

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_KERNEL_EVAL",
        "Evaluate kernel",
        "Stake kernel against difficulty target",
        "kernel.h"
    });

    int64_t newTime = block.nTime;
    bool hit = Stake(tip, stake.get(), tip->nBits, newTime);
    block.nTime = newTime;

    uint256 kernelHash = GetKernelHash(block, tip, stake.get());

    std::ostringstream o;
    o << "{"
      << "\"module\":\"pos\","
      << "\"result\":{"
      << "\"kernel_hit\":" << (hit ? "true" : "false") << ","
      << "\"kernel_hash\":\"" << kernelHash.ToString() << "\","
      << "\"block_time\":" << block.nTime
      << "},"
      << "\"validation\":{"
      << "\"kernel\":{\"checked\":true,\"valid\":true},"
      << "\"difficulty\":{"
      << "\"checked\":true,"
      << "\"bits\":\"" << strprintf("%08x", tip->nBits) << "\","
      << "\"target_hit\":" << (hit ? "true" : "false")
      << "},"
      << "\"economic\":{\"checked\":false,\"reason\":\"Level 1 demo\"}"
      << "},"
      << "\"environment\":{\"level\":1,\"wallet_loaded\":false}"
      << "}";

    g_last = o.str();

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_EXIT",
        "Return result",
        "Return PoS result to UIX",
        "pos.cpp"
    });

    LOG_INFO("POS", "PoS demo complete");

    return g_last.c_str();
}