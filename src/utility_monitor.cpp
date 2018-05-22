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

#include "utility_monitor.h"
#include "hash.h"
#include <fstream>
using namespace std;
#define DEBUG_UMON 0
//#define DEBUG_UMON 1
bool StartDump = false;

UMon::UMon(uint32_t _bankSets, uint32_t _umonLines, uint32_t _buckets) {
    umonLines = _umonLines;
    buckets = _buckets;
    samplingFactor = _bankSets/umonLines;
    sets = umonLines/buckets;

    heads = gm_calloc<Node*>(sets);
    array = gm_calloc<Node*>(sets);
    for (uint32_t i = 0; i < sets; i++) {
        array[i] = gm_calloc<Node>(buckets);
        heads[i] = &array[i][0];
        for (uint32_t j = 0; j < buckets-1; j++) {
            array[i][j].next = &array[i][j+1];
        }
    }

    curWayHits = gm_calloc<uint64_t>(buckets);
    curMisses = 0;

    hf = new H3HashFamily(2, 32, 0xF000BAAD);

    samplingFactorBits = 0;
    uint32_t tmp = samplingFactor;
    while (tmp >>= 1) samplingFactorBits++;

    setsBits = 0;
    tmp = sets;
    while (tmp >>= 1) setsBits++;
}

void UMon::initStats(AggregateStat* parentStat) {
    profWayHits.init("hits", "Sampled hits per bucket", buckets); parentStat->append(&profWayHits);
    profMisses.init("misses", "Sampled misses"); parentStat->append(&profMisses);
}

void UMon::access(Address lineAddr) {
    //1. Hash to decide if it should go in the cache
    uint64_t sampleMask = ~(((uint64_t)-1LL) << samplingFactorBits);
    uint64_t sampleSel = (hf->hash(0, lineAddr)) & sampleMask;

    //info("0x%lx 0x%lx", sampleMask, sampleSel);

    if (sampleSel != 0) {
        return;
    }

    //2. Insert; hit or miss?
    uint64_t setMask = ~(((uint64_t)-1LL) << setsBits);
    uint64_t set = (hf->hash(1, lineAddr)) & setMask;

    // Check hit
    Node* prev = nullptr;
    Node* cur = heads[set];
    bool hit = false;
    for (uint32_t b = 0; b < buckets; b++) {
        if (cur->addr == lineAddr) { //Hit at position b, profile
            //profHits.inc();
            //profWayHits.inc(b);
            curWayHits[b]++;
            hit = true;
            break;
        } else if (b < buckets-1) {
            prev = cur;
            cur = cur->next;
        }
    }

    //Profile miss, kick cur out, put lineAddr in
    if (!hit) {
        curMisses++;
        //profMisses.inc();
        assert(cur->next == nullptr);
        cur->addr = lineAddr;
    }

    //Move cur to MRU (happens regardless of whether this is a hit or a miss)
    if (prev) {
        prev->next = cur->next;
        cur->next = heads[set];
        heads[set] = cur;
    }
}

uint64_t UMon::getNumAccesses() const {
    uint64_t total = curMisses;
    for (uint32_t i = 0; i < buckets; i++) {
        total += curWayHits[buckets - i - 1];
    }
    return total;
}

void UMon::getMisses(uint64_t* misses) {
    uint64_t total = curMisses;
    for (uint32_t i = 0; i < buckets; i++) {
        misses[buckets - i] = total;
        total += curWayHits[buckets - i - 1];
    }
    misses[0] = total;
#if DEBUG_UMON
    info("UMON miss utility curve:");
    for (uint32_t i = 0; i <= buckets; i++) info(" misses[%d] = %ld", i, misses[i]);
#endif
}


void UMon::startNextInterval() {
    curMisses = 0;
    for (uint32_t b = 0; b < buckets; b++) {
        curWayHits[b] = 0;
    }
}

// ==================== ReuseDistSampler class ====================, implemented by shen
ReuseDistSampler::ReuseDistSampler(HashFamily* _hf, uint32_t _bankSets, uint32_t _samplerSets, uint32_t _buckets, uint32_t _max, uint32_t _window) 
    : indices(nullptr), intervalLength(_max), sampleWindow(_window), step((_max + 1) / _buckets), sampleCntrs(nullptr), residuals(nullptr), maxRd(_max),
    dssRate(_bankSets / _samplerSets), samplerSets(_samplerSets), bankSets(_bankSets), hf(_hf)
{
    assert(_bankSets >= _samplerSets);
    assert(intervalLength);
    // the maxRd + 1 should be multiple of RDV size, so that the rdv can be filled up by all RDs
    assert((maxRd + 1) % _buckets == 0);

    indices = gm_calloc<uint32_t>(samplerSets); // for the index counters
    if (_window) {
        sampleCntrs = gm_calloc<uint32_t>(samplerSets); // for the index counters
        residuals = gm_calloc<uint32_t>(samplerSets); // for the index counters
    }
    
    rdvSize = _buckets;

    info("ReuseDistSampler: sampling rate 1/%d, # of sampler sets %d, max RD %d, buckets: %d, sampling window %d", 
        dssRate, samplerSets, _max, rdvSize, _window);
}

ReuseDistSampler::~ReuseDistSampler() 
{ 
    gm_free(indices);
    
    if (sampleCntrs != nullptr && residuals != nullptr) {
        gm_free(sampleCntrs);
        gm_free(residuals);
    }
    //delete rdv;
};


void ReuseDistSampler::initStats(AggregateStat* parentStat) {
    AggregateStat* rdSamplerStat = new AggregateStat();
    rdSamplerStat->init("rdSamplerStat", "reuse distance sampler stats");
    rdv.init("rdd", "reuse distance distribution", rdvSize); rdSamplerStat->append(&rdv);
    parentStat->append(rdSamplerStat);
}

/*
// delete the old entry wholse RD is larger than maxRd 
uint32_t ReuseDistSampler::cleanOldEntry()
{
    // the total number of rd-counters whose values are larger then truncation 
    uint32_t maxNum = 0;

    for (auto it = addrMap.begin(); it != addrMap.end(); ) {
        uint64_t set = hf->hash(0, it->first) & (bankSets - 1); // do not need the mask
        uint32_t ss = set / samplerSets; // ss is the sampler set index
        assert(ss < samplerSets);
        int32_t diff = (int32_t)index - it->second - 1;
        diff = diff < 0 ? diff + intervalLength : diff;
        if (diff >= (int)maxRd) {
            ++maxNum;
            auto eraseIt = it;
            ++it;
            addrMap.erase(eraseIt);
        }
        else ++it;
    }

    // if now is in sampling mode, each entry accounts for a sampled address,
    // so the corresponding element in RDV should be increased when old entry deleting
    rdv.sample(DOLOG(maxRd), maxNum);
    return maxNum;
}*/

int32_t ReuseDistSampler::access(Address addr)
{
    if (unlikely(StartDump)) { // check the dump flag
        clear();
        StartDump = false;
    }

    uint32_t set = bankSets == 1 ? 0 : getSet(addr); // when the cache is fully associative, set index = 0.
    // do we sample this line? if not, just return
    if (set & (dssRate - 1))
        return -1;

    uint32_t ss = set / dssRate; // ss is the sampler set index
    assert(ss < samplerSets);

    uint32_t & index = ++indices[ss];

    auto pos = addrMap.find(addr);
    int32_t rd = -1;

    if (pos != addrMap.end()) {
        rd = index - pos->second - 1; 
        // if the rd larger than the truncation, we limite the maximum RD to maxRd
        rd = DOLOG(rd > (int32_t)maxRd ? maxRd : rd) / step;
        assert(rd < (int32_t)rdv.size());
        // the first step rds account for the increment of rdv[0], the second ones for the increment of rdv[1], and so on
        rdv.inc(rd);
        // for the sampling scheme, rd-counter is clear when finish rd calculation
        if (sampleWindow)
            addrMap.erase(pos);
    }

    // for sampline scheme 
    if (sampleWindow && sampleCntrs[ss])
        --sampleCntrs[ss];
    // when sampleCntr is zero in sampling scheme, we sample an address to calculate rd.
    else if (sampleWindow) {
        // ensure no same address is existing in addrMap 
        assert(!addrMap[addr]);
        uint32_t random = rng.randInt((uint64_t)sampleWindow);
        // set sampleCntr for the next sample 
        sampleCntrs[ss] = random + residuals[ss];
        residuals[ss] = sampleWindow - random;

        // record the new sampled address and the index 
        addrMap[addr] = index; 
    }
    // for non-sampling scheme 
    else 
        addrMap[addr] = index;
    
    //info("access rd sampler, size: %d, rdv samples: %d", (int)addrMap.size(), rdv->getSamples());
    return rd;
}

void ReuseDistSampler::clear() {
    info("cleaning, addrMap size: %d", (int)addrMap.size());
    addrMap.clear(); 

    for (uint32_t i = 0; i < samplerSets; ++i) {
        indices[i] = 0;
        if (sampleWindow) {
            sampleCntrs[i] = 0;
            residuals[i] = 0;
        }
    }
}
