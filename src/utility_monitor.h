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
// add by shen
#include "mtrand.h"
#include "g_std/g_unordered_map.h"

//Print some information regarding utility monitors and partitioning
#define UMON_INFO 1
//#define UMON_INFO 1

class HashFamily;

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

/* for recording distribution into a Histogram, 
   Accur is the accuracy of transforming calculation */
template <class B = uint32_t>
class Histogram : public GlobAlloc
{
    protected:
        B * bins;
        B samples;
        int size;

    public:
        Histogram() : bins(nullptr), samples(0), size(0) {};
    
        Histogram(int s);
    
        Histogram(const Histogram<B> & rhs);
    
        ~Histogram();
    
        void setSize(int s);
    
        const int getSize();
    
        void clear();
    
        void normalize();
    
        B & operator[] (const int idx) const;
    
        Histogram<B> & operator=(const Histogram<B> & rhs);
    
        Histogram<B> & operator+=(const Histogram<B> & rhs);
    
        inline void sample(int x, uint16_t n = 1);
    
        const B getSamples() const; 
    
        //void print(std::ofstream & file);
        void print();
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

/* do reuse distance statistics */
class ReuseDistSampler : public GlobAlloc 
{
    private:
        g_unordered_map<uint64_t, uint32_t> addrMap;
        uint32_t* indices;
        uint32_t intervalLength; // must be 2^n
        uint32_t sampleWindow;
        uint32_t* sampleCntrs;
        uint32_t* residuals;
        uint32_t maxRd;
        uint32_t dssRate; // we sample one set every "dssRate" sets, must be 2^n
        uint32_t samplerSets;
        uint32_t bankSets;
        MTRand rng;
        Histogram<uint32_t> rdv;
        //Used in masks for set indices and sampling factor descisions
        HashFamily* hf; // we calculate the RD for each set, so the hash function using in cache array is needed
    public:
        ReuseDistSampler(uint32_t _bankSets, uint32_t _samplerSets, uint32_t _intLength, uint32_t _window = 0);
    
        ~ReuseDistSampler();
    
        uint32_t cleanOldEntry();
    
        //uint32_t mapMaxSize() { return maxsize; }
    
        void access(uint64_t addr);
    
        void clear();
};

#endif  // UTILITY_MONITOR_H_

