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

#ifndef UTILITY_MONITOR_H_
#define UTILITY_MONITOR_H_

#include "galloc.h"
#include "memory_hierarchy.h"
#include "stats.h"
#include "hash.h"
// add by shen
#include "mtrand.h"
#include "g_std/g_unordered_map.h"

//Print some information regarding utility monitors and partitioning
//#define UMON_INFO 1

extern bool StartDump; // the dump flag, add by shen

//class HashFamily;

class UMon : public GlobAlloc {
    private:
        uint32_t umonLines;
        uint32_t samplingFactor; //Size of sampled cache (lines)/size of umon. Should be power of 2
        uint32_t buckets; //umon ways
        uint32_t sets; //umon sets. Should be power of 2.

        //Used in masks for set indices and sampling factor descisions
        uint64_t samplingFactorBits;
        uint64_t setsBits;

        uint64_t* curWayHits;
        uint64_t curMisses;

        Counter profHits;
        Counter profMisses;
        VectorCounter profWayHits;

        //Even for high associativity/number of buckets, performance of this is not important because we downsample so much (so this is a LL)
        struct Node {
            Address addr;
            struct Node* next;
        };
        Node** array;
        Node** heads;

        HashFamily* hf;

    public:
        UMon(uint32_t _bankLines, uint32_t _umonLines, uint32_t _buckets);
        void initStats(AggregateStat* parentStat);

        void access(Address lineAddr);

        uint64_t getNumAccesses() const;
        void getMisses(uint64_t* misses);
        void startNextInterval();

        uint32_t getBuckets() const { return buckets; }
};

/* calculate log2(s) + 1 */
template<class T>
inline T log2p1(T s)
{
    T result = 0;
    while (s) {
        s >>= 1;
        ++result;
    }

    return result;
}

/* fast to calculate log2(x)+1 */
#ifdef LOG2
#define DOLOG(x) log2p1(x)
#else
#define DOLOG(x) x
#endif

/* do reuse distance statistics, details in MICRO 2012 PDP paper, add by shen */
class ReuseDistSampler : public GlobAlloc 
{
    private:
        g_unordered_map<uint64_t, uint32_t> addrMap;
        uint32_t* indices;
        uint32_t intervalLength; // must be 2^n
        uint32_t sampleWindow;
        uint32_t step; // step counter, Sc in the PDP peper
        uint32_t* sampleCntrs;
        uint32_t* residuals;
        uint32_t maxRd; // it should be 2^n - 1, all RDs range in [0, maxRd]
        uint32_t dssRate; // we sample one set every "dssRate" sets, must be 2^n
        uint32_t samplerSets;
        uint32_t bankSets;
        MTRand rng;
        VectorCounter rdv; // the maxRd + 1 should be multiple of RDV size, so that the rdv can be filled up by all RDs
        uint32_t rdvSize;
        //Used in masks for set indices and sampling factor descisions
        HashFamily* hf; // we calculate the RD for each set, so the hash function using in cache array is needed
    public:
        ReuseDistSampler(HashFamily* _hf, uint32_t _bankSets, uint32_t _samplerSets, uint32_t _buckets, uint32_t _max, uint32_t _window);
    
        ~ReuseDistSampler();

        void initStats(AggregateStat* parentStat);

        inline const uint32_t getSet(uint64_t addr) { return hf->hash(0, addr) & (bankSets - 1); }
    
        uint32_t cleanOldEntry();
    
        //uint32_t mapMaxSize() { return maxsize; }
    
        uint32_t access(Address addr);
    
        void print();

        void clear();

        uint32_t getStep() const { return step; }

        inline const uint32_t getRdvBin(uint32_t b) { return rdv.count(b); }
        const uint32_t getRdvSize() { return rdvSize; }
};

#endif  // UTILITY_MONITOR_H_

