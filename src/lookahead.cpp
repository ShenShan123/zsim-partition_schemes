/** $lic$
 * Copyright (C) 2012-2015 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <tuple>
#include "part_repl_policies.h"
#include "partitioner.h"

using std::tuple;
using std::tie;
using std::make_tuple;

// generic lookahead algorithm
namespace lookahead {

static tuple<double, uint32_t> getMaxMarginalUtility(
    uint32_t numPartitions, uint32_t part, uint32_t partAlloc,
    uint32_t balance, const PartitionMonitor& monitor) {
    double maxMu = -1.0;
    uint32_t maxMuAlloc = 0;
    for (uint32_t i = 1; i <= balance; i++) {
        //Use this when utility == misses
        uint64_t extraHits = monitor.get(part, partAlloc) - monitor.get(part, partAlloc+i);
        double mu = ((double)extraHits)/((double)i);

        if (mu > maxMu) {
            maxMu = mu;
            maxMuAlloc = i;
        }
    }
    return make_tuple(maxMu, maxMuAlloc);
}

//Utility is defined as misses saved over not having a cache
uint64_t computePartitioningTotalUtility(
    uint32_t numPartitions, const uint32_t* parts, const PartitionMonitor& monitor) {
    uint64_t noCacheMisses = 0;
    uint64_t curPartMisses = 0;
    for (uint32_t p = 0; p < numPartitions; p++) {
        noCacheMisses += monitor.get(p, 0);
        curPartMisses += monitor.get(p, parts[p]);
    }
    return noCacheMisses - curPartMisses;
}

void computeBestPartitioning(
    uint32_t numPartitions, uint32_t buckets, uint32_t minAlloc, bool* forbidden,
    uint32_t* allocs, const PartitionMonitor& monitor) {
    uint32_t balance = buckets;

    // Zero out allocs or set to mins
    for (uint32_t i = 0; i < numPartitions; i++) {
        allocs[i] = minAlloc / numPartitions;
        assert(allocs[i]);
    }

    balance -= minAlloc;

    uint32_t iter = 0;  // purely for debug purposes
    while (balance > 0) {
        double maxMu = -1.0;
        uint32_t maxMuPart = numPartitions;  // illegal
        uint32_t maxMuAlloc = 0;
        (void)maxMuAlloc;  // make gcc happy when we're not profiling
        for (uint32_t i = 0; i < numPartitions; i++) {
            if (forbidden && forbidden[i]) {  // this partition doesn't get anything
                //info("Allocating to %d forbiddden, skipping", i);
                continue;
            }

            uint32_t muAlloc;
            double mu;
            tie(mu, muAlloc) = getMaxMarginalUtility(numPartitions, i, allocs[i], balance, monitor);
            if (mu > maxMu) {
                maxMu = mu;
                maxMuPart = i;
                maxMuAlloc = muAlloc;
            }
        }
#if UMON_INFO
        //info("LookaheadPartitioner: Iteration %d maxMu %f partition %d alloc %d newAlloc %d remaining %d", iter, maxMu, maxMuPart, maxMuAlloc, allocs[maxMuPart] + maxMuAlloc, balance - maxMuAlloc);
#endif
        assert(maxMuPart < numPartitions);
        allocs[maxMuPart] += maxMuAlloc;
        balance -= maxMuAlloc;
        iter++;
    }
    uint32_t tot = 0;

    for (uint32_t i = 0; i < numPartitions; ++i) {
        tot += allocs[i];
    }
}

}  // namespace lookahead

// LookaheadPartitioner

LookaheadPartitioner::LookaheadPartitioner(PartReplPolicy* _repl, uint32_t _numPartitions, uint32_t _buckets,
                                           uint32_t _minAlloc, double _allocPortion, bool* _forbidden)
        : Partitioner(_minAlloc, _allocPortion, _forbidden)
        , repl(_repl)
        , numPartitions(_numPartitions)
        , buckets(_buckets) {
    assert_msg(buckets > 0, "Must have non-zero buckets to avoid divide-by-zero exception.");

    curAllocs = gm_calloc<uint32_t>(buckets + 1);

    info("LookaheadPartitioner: %d part buckets", buckets);
}

//allocs are in buckets
void LookaheadPartitioner::partition() {
    
    auto& monitor = *repl->getMonitor();

    uint32_t bestAllocs[numPartitions];
    lookahead::computeBestPartitioning(
        numPartitions, allocPortion*buckets, minAlloc*numPartitions,
        forbidden, bestAllocs, monitor);

    uint64_t newUtility = lookahead::computePartitioningTotalUtility(
        numPartitions, bestAllocs, monitor);
    uint64_t curUtility = lookahead::computePartitioningTotalUtility(
        numPartitions, curAllocs, monitor);

    bool switchAllocs = newUtility > 102*curUtility/100; //must be 2% better
    if (curUtility == 0) switchAllocs = true; //always switch on start (this happens when we only have recorded misses)
    switchAllocs = true; //FIXME

    if (switchAllocs) {
#if UMON_INFO
        info("LookaheadPartitioner: Switching allocation, new util %ld, old util %ld", newUtility, curUtility);
#endif
        //std::copy(bestAllocs, bestAllocs+numPartitions, curAllocs);
    } else {
#if UMON_INFO
        info("LookaheadPartitioner: KEEPING allocation, new util %ld, old util %ld", newUtility, curUtility);
#endif
    }

#if UMON_INFO
    info("LookaheadPartitioner: Partitioning done,");
    for (uint32_t i = 0; i < numPartitions; i++) info("buckets[%d] = %d", i, curAllocs[i]);
#endif

    repl->setPartitionSizes(curAllocs);
    repl->getMonitor()->reset();

}


// ============================= Hit Maximization Partitioner, added by shen =============================
HitMaxPartitioner::HitMaxPartitioner(PartReplPolicy* _repl, uint32_t _numPartitions, uint32_t _buckets, uint32_t _minAlloc, double _allocPortion, bool* _forbidden)
        : LookaheadPartitioner(_repl, _numPartitions, _buckets, _minAlloc, _allocPortion, _forbidden) {
    info("HitMaxPartitioner uses the LoolaheadPartitioner");
}

void HitMaxPartitioner::partition() {
    uint64_t totalGain = 0;
    uint64_t potentialGain[numPartitions];

    auto& monitor = *repl->getMonitor();

    uint64_t standAloneMisses = 0;
    uint64_t curPartMisses = 0;

    for (uint32_t p = 0; p < numPartitions; p++) {
        standAloneMisses = monitor.get(p, buckets - 1);
        curPartMisses = monitor.get(p, curAllocs[p]);
        assert(standAloneMisses <= curPartMisses);
        potentialGain[p] = curPartMisses - standAloneMisses;
        totalGain += potentialGain[p];
    }

    double bestAllocsFrac[numPartitions]; // Tcore in the paper
    double totAlloc = 0.0;

    for (uint32_t p = 0; p < numPartitions; p++) {
        bestAllocsFrac[p] = curAllocs[p] * (1.0 + (double)potentialGain[p] / totalGain);
        totAlloc += bestAllocsFrac[p];
    }

    uint32_t balance = buckets;

    for (uint32_t p = 0; p < numPartitions; p++) {
        bestAllocsFrac[p] /= totAlloc; // normalize the Tcore
        curAllocs[p] = bestAllocsFrac[p] * buckets;

        if (!curAllocs[p]) // if the partition has no resource
            curAllocs[p] = buckets / numPartitions;
        
        balance -= curAllocs[p];
        //info("partition %d allocates %d buckets", p, curAllocs[p]);
    }

    assert(balance <= buckets);
    curAllocs[0] += balance; // add the residual caused by float point calculation to the first part
    //info("partition 0 has the balance %d", balance);

    repl->setPartitionSizes(curAllocs);
    repl->getMonitor()->reset();
}
// end