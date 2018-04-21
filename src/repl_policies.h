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

#ifndef REPL_POLICIES_H_
#define REPL_POLICIES_H_

#include <functional>
#include "bithacks.h"
#include "cache_arrays.h"
#include "coherence_ctrls.h"
#include "memory_hierarchy.h"
#include "mtrand.h"
#include "utility_monitor.h" // add by shen

/* Generic replacement policy interface. A replacement policy is initialized by the cache (by calling setTop/BottomCC) and used by the cache array. Usage follows two models:
 * - On lookups, update() is called if the replacement policy is to be updated on a hit
 * - On each replacement, rank() is called with the req and a list of replacement candidates.
 * - When the replacement is done, replaced() is called. (See below for more detail.)
 */
class ReplPolicy : public GlobAlloc {
    protected:
        CC* cc; //coherence controller, used to figure out whether candidates are valid or number of sharers

    public:
        uint64_t* array; // 为了使父类中的函数合法

        ReplPolicy() : cc(nullptr) {}

        virtual void setCC(CC* _cc) {cc = _cc;}

        virtual void update(uint32_t id, const MemReq* req) = 0;

        // we add this func to implement RRIP repl, take a note that all sub-class inherited this must override this method, by sxj
        virtual void hitUpdate(uint32_t id, const MemReq* req) = 0; 
        virtual void replaced(uint32_t id) = 0;

        virtual uint32_t rankCands(const MemReq* req, SetAssocCands cands) = 0;
        virtual uint32_t rankCands(const MemReq* req, ZCands cands) = 0;

        virtual void initStats(AggregateStat* parent) {}
};

/* Add DECL_RANK_BINDINGS to each class that implements the new interface,
 * then implement a single, templated rank() function (see below for examples)
 * This way, we achieve a simple, single interface that is specialized transparently to each type of array
 * (this code is performance-critical)
 */
#define DECL_RANK_BINDING(T) uint32_t rankCands(const MemReq* req, T cands) { return rank(req, cands); }
#define DECL_RANK_BINDINGS DECL_RANK_BINDING(SetAssocCands); DECL_RANK_BINDING(ZCands);

/* Legacy support.
 * - On each replacement, the controller first calls startReplacement(), indicating the line that will be inserted;
 *   then it calls recordCandidate() for each candidate it finds; finally, it calls getBestCandidate() to get the
 *   line chosen for eviction. When the replacement is done, replaced() is called. The division of getBestCandidate()
 *   and replaced() happens because the former is called in preinsert(), and the latter in postinsert(). Note how the
 *   same restrictions on concurrent insertions extend to this class, i.e. startReplacement()/recordCandidate()/
 *   getBestCandidate() will be atomic, but there may be intervening update() calls between getBestCandidate() and
 *   replaced().
 */
class LegacyReplPolicy : public virtual ReplPolicy {
    protected:
        virtual void startReplacement(const MemReq* req) {} //many policies don't need it
        virtual void recordCandidate(uint32_t id) = 0;
        virtual uint32_t getBestCandidate() = 0;

    public:
        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
            startReplacement(req);
            for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                recordCandidate(*ci);
            }
            return getBestCandidate();
        }

        DECL_RANK_BINDINGS;
};

/* Plain ol' LRU, though this one is sharers-aware, prioritizing lines that have
 * sharers down in the hierarchy vs lines not shared by anyone.
 */
template <bool sharersAware>
class LRUReplPolicy : public ReplPolicy {
    protected:
        uint64_t timestamp; // incremented on each access
        uint64_t* array;
        uint32_t numLines;

    public:
        explicit LRUReplPolicy(uint32_t _numLines) : timestamp(1), numLines(_numLines) {
            array = gm_calloc<uint64_t>(numLines);
        }

        ~LRUReplPolicy() {
            gm_free(array);
        }

        void update(uint32_t id, const MemReq* req) {
            array[id] = timestamp++;
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } // sxj

        void replaced(uint32_t id) {
            array[id] = 0;
        }

        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
            uint32_t bestCand = -1;
            uint64_t bestScore = (uint64_t)-1L;
            for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                uint32_t s = score(*ci);
                bestCand = (s < bestScore)? *ci : bestCand;
                bestScore = MIN(s, bestScore);
            }
            return bestCand;
        }

        DECL_RANK_BINDINGS;

    private:
        inline uint64_t score(uint32_t id) { //higher is least evictable
            //array[id] < timestamp always, so this prioritizes by:
            // (1) valid (if not valid, it's 0)
            // (2) sharers, and
            // (3) timestamp
            return (sharersAware? cc->numSharers(id) : 0)*timestamp + array[id]*cc->isValid(id);
        }
};

//RRIP 
template <bool sharersAware>
class SRRIPReplPolicy : public ReplPolicy {
    protected:
        uint8_t* array; // 用于RRPV的存储
        uint32_t numLines; //line的数量
        uint8_t distantRRPV; // for SRRIP, the best is M=4bits, so the "long re-reference interval" is RRPV = 14

    public:
        explicit SRRIPReplPolicy(uint32_t _numLines, const uint32_t _dist) : numLines(_numLines), distantRRPV(_dist) {
            array = gm_calloc<uint8_t>(numLines);

            for (uint32_t i = 0; i <= numLines; i++)
                array[i] = distantRRPV; // 一开始对所有的line的RRPV初始化为distant RRPV

            info("init SRRIP replacement policy, distant prediction is %d", distantRRPV);
        }

        ~SRRIPReplPolicy() {
            gm_free(array);
        }

        void update(uint32_t id, const MemReq* req) {
            array[id] = distantRRPV - 1;
        }

        //this function only works differently from update() in RRIP repl policy, sxj
        void hitUpdate(uint32_t id, const MemReq* req) { // sxj
            array[id] = 0;
        }

        void replaced(uint32_t id) {
            array[id] = distantRRPV - 1; // if the line has been replaced, its RRPV is long
        }

        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
            uint32_t bestCand = numLines;
            uint8_t bestScore = (uint8_t)-1;
            bool haveA3 = false; //the flag that successfully finds a victim
            while (!haveA3) { // 如果最终都没有没有sharer且RRPV为distant的line，则set内的line的RRPV+1
                for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                    uint32_t s = score(*ci);
                    bestCand = (s < bestScore)? *ci : bestCand;
                    bestScore = MIN(s, bestScore); // lower score the better candidate
                    if (!bestScore) { // the candidate has been found till the best score is 0
                        haveA3 = true;
                        break;
                    }
                }

                if (!haveA3) // if no candidates with RRPV = distant re-reference, increment all lines in the set
                    for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                        assert(array[*ci] < distantRRPV);
                        ++array[*ci];
                    }
            }

            return bestCand;
        }

        DECL_RANK_BINDINGS;

    private:
        inline uint8_t score(uint32_t id) { //higher is least evictable
            //array[id] < timestamp always, so this prioritizes by:
            // (1) valid (if not valid, it's 0)
            // (2) sharers, and
            // (3) timestamp
            //return (sharersAware? cc->numSharers(id) : 0) * distantRRPV + (distantRRPV - array[id])*cc->isValid(id); // a samll value implies a better cand
            return (distantRRPV - array[id])*cc->isValid(id); // at some circumstances, all lines are sharers, so there is no candidates met the condition of evicting 
        }
};


// This is the PDP replacement policy, read MICRO 2012 paper for more detail, by shen
class PDPReplPolicy : public LegacyReplPolicy {
    protected:
        struct LineInfo {
            uint8_t rpd;
            bool reuse; // the reuse bit
        };

        LineInfo* array; // store the RPDs for all lines
        uint32_t numLines;
        uint32_t assoc;
        uint32_t* candArray;
        uint32_t* distanceStep; // Sd in the paper, one counter per set
        uint32_t numCands;
        uint32_t candIdx;
        ReuseDistSampler* rdSampler;
        uint8_t pd; // the max pd is 254, must less than rdv size!!
        int32_t bestId;
        uint32_t maxSd; // the maximum distance step, default value is 8
        uint32_t totAccs; // total accesses, Nt
        bool nonInclusive;
        uint32_t period;

    public:
        PDPReplPolicy(uint32_t _numLines, uint32_t _numSets, uint32_t _assoc, uint32_t _numCands, ReuseDistSampler* _rdSampler, uint32_t _maxSd, bool _nonInclusive) 
        : numLines(_numLines), assoc(_assoc), numCands(_numCands), rdSampler(_rdSampler), 
        pd((uint8_t)-1), bestId(-1), maxSd(_maxSd), totAccs(0), nonInclusive(_nonInclusive)
        {
            array = gm_calloc<LineInfo>(numLines);
            for (uint32_t i = 0; i < numLines; i++) {
                array[i].rpd = pd;
                array[i].reuse = false;
            }

            candArray = gm_calloc<uint32_t>(numCands);
            distanceStep = gm_calloc<uint32_t>(_numSets);
            period = 512 * 1024;
            info("PDP replacement policy, max PD %d, max distance %d, pd computing period %d", pd, maxSd, period);
        }

        ~PDPReplPolicy() {
            gm_free(array);
            gm_free(candArray);
            gm_free(distanceStep);
            delete rdSampler;
        }

        void update(uint32_t id, const MemReq* req) {
            ++totAccs;
            rdSampler->access(req->lineAddr);
            uint32_t set = rdSampler->getSet(req->lineAddr);
            ++distanceStep[set]; // every access, the distance step counter increase by 1
            uint32_t first = set * assoc;
            // if the distance step is maxSd, we decrement all RPDs of lines in this set
            if (distanceStep[set] >= maxSd - 1) {
                distanceStep[set] = 0;
                for (uint32_t id = first; id < assoc + first; ++id)
                    array[id].rpd -= array[id].rpd != 0; // decreae every access to this set, the lines' saturating counter decrease
            }
        }

        void hitUpdate(uint32_t id, const MemReq* req) { 
            array[id].reuse = true; 
            array[id].rpd = pd;

            update(id, req);
        }
        
        void replaced(uint32_t id) {
            array[id].rpd = pd; // when replaced, the array's RPD counter is set to pd
            array[id].reuse = false;
            bestId = -1;
            candIdx = 0;
        }
        
        void recordCandidate(uint32_t id) {
            candArray[candIdx++] = id;
        }

        uint32_t getBestCandidate() {
            uint32_t bestRpd = 0;
            uint32_t bestInsrtRpd = 0;
            int32_t bestProt = -1;
            int32_t bestInsert = -1;
            for (uint32_t i = 0; i < candIdx; ++i) {
                LineInfo & cand = array[candArray[i]];
                if (cand.rpd == 0) {
                    bestId = candArray[i];
                    break; // if we found the unprotected line, just break
                }
                else if (!cand.reuse && cand.rpd > bestInsrtRpd) { // there is a inserted line with no reuse
                    bestInsert = candArray[i];
                    bestInsrtRpd = cand.rpd;
                }
                else if (cand.rpd > bestRpd) { // the line with largest PD
                    bestProt = candArray[i];
                    bestRpd = cand.rpd;
                }
            }

            if (bestId == -1 && nonInclusive) // if this is a non-inclusive cache, just bypoass this access
                bestId = numLines;

            bestId = bestId == -1 ? (bestInsert == -1 ? bestProt : bestInsert) : bestId;
            assert(bestId != -1);
            //info("best line %d, best inserted line %d, best protected line %d", bestId, bestInsert, bestProt);

            // calculate the best PD every 512K access
            if (totAccs < period)
                return bestId;
            // E(dp) = sum i=1->dp (Ni) / {sum i=1->dp (Ni*i) + (Nt - sum i=1->dp (Ni*i) * (dp + de))}
            uint64_t sumN = 0;
            uint64_t sumNi = 0;
            double emax = 0;
            uint8_t pdBest = 0;
            uint32_t step = rdSampler->getStep();
            for (uint8_t d = 1; d < (uint8_t)-1; ++d) {
                uint32_t idx = d / step;
                uint32_t v = rdSampler->getRdvBin(idx) / step; // one bin cotains the RD ranging in [d * step, d * step + step)
                //info("the rdv[%d] = %d", d, v);
                sumN += v;
                sumNi += v * d;
                assert((int32_t)totAccs - sumN >= 0); // the number of long RD (> maxRd) cannot be negative
                uint64_t denominator = sumNi + (totAccs - sumN) * (d + assoc);
                double e = (double)sumN / denominator;

                if (e > emax) {
                    emax = e;
                    pdBest = d;
                }
            }

            assert(pdBest);

            pd = pdBest;
            
            totAccs = 0;
            rdSampler->clear();
            
            info("best PD = %d, hit rate per way = %f", pdBest, emax);

            return bestId;
        }

        DECL_RANK_BINDINGS;
};

//This is VERY inefficient, uses LRU timestamps to do something that in essence requires a few bits.
//If you want to use this frequently, consider a reimplementation
class TreeLRUReplPolicy : public LRUReplPolicy<true> {
    private:
        uint32_t* candArray;
        uint32_t numCands;
        uint32_t candIdx;

    public:
        TreeLRUReplPolicy(uint32_t _numLines, uint32_t _numCands) : LRUReplPolicy<true>(_numLines), numCands(_numCands), candIdx(0) {
            candArray = gm_calloc<uint32_t>(numCands);
            if (numCands & (numCands-1)) panic("Tree LRU needs a power of 2 candidates, %d given", numCands);
        }

        ~TreeLRUReplPolicy() {
            gm_free(candArray);
        }

        void recordCandidate(uint32_t id) {
            candArray[candIdx++] = id;
        }

        uint32_t getBestCandidate() {
            assert(candIdx == numCands);
            uint32_t start = 0;
            uint32_t end = numCands;

            while (end - start > 1) {
                uint32_t pivot = start + (end - start)/2;
                uint64_t t1 = 0;
                uint64_t t2 = 0;
                for (uint32_t i = start; i < pivot; i++) t1 = MAX(t1, array[candArray[i]]);
                for (uint32_t i = pivot; i < end; i++)   t2 = MAX(t2, array[candArray[i]]);
                if (t1 > t2) start = pivot;
                else end = pivot;
            }
            //for (uint32_t i = 0; i < numCands; i++) printf("%8ld ", array[candArray[i]]);
            //info(" res: %d (%d %ld)", start, candArray[start], array[candArray[start]]);
            return candArray[start];
        }

        void replaced(uint32_t id) {
            candIdx = 0;
            array[id] = 0;
        }
};

//2-bit NRU, see A new Case for Skew-Associativity, A. Seznec, 1997
class NRUReplPolicy : public LegacyReplPolicy {
    private:
        //read-only
        uint32_t* array;
        uint32_t* candArray;
        uint32_t numLines;
        uint32_t numCands;

        //read-write
        uint32_t youngLines;
        uint32_t candVal;
        uint32_t candIdx;

    public:
        NRUReplPolicy(uint32_t _numLines, uint32_t _numCands) :numLines(_numLines), numCands(_numCands), youngLines(0), candIdx(0) {
            array = gm_calloc<uint32_t>(numLines);
            candArray = gm_calloc<uint32_t>(numCands);
            candVal = (1<<20);
        }

        ~NRUReplPolicy() {
            gm_free(array);
            gm_free(candArray);
        }

        void update(uint32_t id, const MemReq* req) {
            //if (array[id]) info("update PRE %d %d %d", id, array[id], youngLines);
            youngLines += 1 - (array[id] >> 1); //+0 if young, +1 if old
            array[id] |= 0x2;

            if (youngLines >= numLines/2) {
                //info("youngLines = %d, shifting", youngLines);
                for (uint32_t i = 0; i < numLines; i++) array[i] >>= 1;
                youngLines = 0;
            }
            //info("update POST %d %d %d", id, array[id], youngLines);
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } // sxj

        void recordCandidate(uint32_t id) {
            uint32_t iVal = array[id];
            if (iVal < candVal) {
                candVal = iVal;
                candArray[0] = id;
                candIdx = 1;
            } else if (iVal == candVal) {
                candArray[candIdx++] = id;
            }
        }

        uint32_t getBestCandidate() {
            assert(candIdx > 0);
            return candArray[youngLines % candIdx]; // youngLines used to sort-of-randomize
        }

        void replaced(uint32_t id) {
            //info("repl %d val %d cands %d", id, array[id], candIdx);
            candVal = (1<<20);
            candIdx = 0;
            array[id] = 0;
        }
};

class RandReplPolicy : public LegacyReplPolicy {
    private:
        //read-only
        uint32_t* candArray;
        uint32_t numCands;

        //read-write
        MTRand rnd;
        uint32_t candVal;
        uint32_t candIdx;

    public:
        explicit RandReplPolicy(uint32_t _numCands) : numCands(_numCands), rnd(0x23A5F + (uint64_t)this), candIdx(0) {
            candArray = gm_calloc<uint32_t>(numCands);
        }

        ~RandReplPolicy() {
            gm_free(candArray);
        }

        void update(uint32_t id, const MemReq* req) {}

        void hitUpdate(uint32_t id, const MemReq* req) {} // sxj

        void recordCandidate(uint32_t id) {
            candArray[candIdx++] = id;
        }

        uint32_t getBestCandidate() {
            assert(candIdx == numCands);
            uint32_t idx = rnd.randInt(numCands-1);
            return candArray[idx];
        }

        void replaced(uint32_t id) {
            candIdx = 0;
        }
};

class LFUReplPolicy : public LegacyReplPolicy {
    private:
        uint64_t timestamp; // incremented on each access
        int32_t bestCandidate; // id
        struct LFUInfo {
            uint64_t ts;
            uint64_t acc;
        };
        LFUInfo* array;
        uint32_t numLines;

        //NOTE: Rank code could be shared across Replacement policy implementations
        struct Rank {
            LFUInfo lfuInfo;
            uint32_t sharers;
            bool valid;

            void reset() {
                valid = false;
                sharers = 0;
                lfuInfo.ts = 0;
                lfuInfo.acc = 0;
            }

            inline bool lessThan(const Rank& other, const uint64_t curTs) const {
                if (!valid && other.valid) {
                    return true;
                } else if (valid == other.valid) {
                    if (sharers == 0 && other.sharers > 0) {
                        return true;
                    } else if (sharers > 0 && other.sharers == 0) {
                        return false;
                    } else {
                        if (lfuInfo.acc == 0) return true;
                        if (other.lfuInfo.acc == 0) return false;
                        uint64_t ownInvFreq = (curTs - lfuInfo.ts)/lfuInfo.acc; //inverse frequency, lower is better
                        uint64_t otherInvFreq = (curTs - other.lfuInfo.ts)/other.lfuInfo.acc;
                        return ownInvFreq > otherInvFreq;
                    }
                }
                return false;
            }
        };

        Rank bestRank;

    public:
        explicit LFUReplPolicy(uint32_t _numLines) : timestamp(1), bestCandidate(-1), numLines(_numLines) {
            array = gm_calloc<LFUInfo>(numLines);
            bestRank.reset();
        }

        ~LFUReplPolicy() {
            gm_free(array);
        }

        void update(uint32_t id, const MemReq* req) {
            //ts is the "center of mass" of all the accesses, i.e. the average timestamp
            array[id].ts = (array[id].acc*array[id].ts + timestamp)/(array[id].acc + 1);
            array[id].acc++;
            timestamp += 1000; //have larger steps to avoid losing too much resolution over successive divisions
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } // sxj

        void recordCandidate(uint32_t id) {
            Rank candRank = {array[id], cc? cc->numSharers(id) : 0, cc->isValid(id)};

            if (bestCandidate == -1 || candRank.lessThan(bestRank, timestamp)) {
                bestRank = candRank;
                bestCandidate = id;
            }
        }

        uint32_t getBestCandidate() {
            assert(bestCandidate != -1);
            return (uint32_t)bestCandidate;
        }

        void replaced(uint32_t id) {
            bestCandidate = -1;
            bestRank.reset();
            array[id].acc = 0;
        }
};

//Extends a given replacement policy to profile access ordering violations
template <class T>
class ProfViolReplPolicy : public T {
    private:
        struct AccTimes {
            uint64_t read;
            uint64_t write;
        };

        AccTimes* accTimes;

        Counter profRAW, profWAR, profRAR, profWAW, profNoViolAcc;
        Counter profAAE, profNoViolEv; //access after eviction violation

        uint64_t replCycle;

    public:
        //using T::T; //C++11, but can't do in gcc yet

        //Since this is only used with LRU, let's do that...
        explicit ProfViolReplPolicy(uint32_t nl) : T(nl) {}

        void init(uint32_t numLines) {
            accTimes = gm_calloc<AccTimes>(numLines);
            replCycle = 0;
        }

        void initStats(AggregateStat* parentStat) {
            T::initStats(parentStat);
            profRAW.init("vRAW", "RAW violations (R simulated before preceding W)");
            profWAR.init("vWAR", "WAR violations (W simulated before preceding R)");
            profRAR.init("vRAR", "RAR violations (R simulated before preceding R)");
            profWAW.init("vWAW", "WAW violations (W simulated before preceding W)");
            profAAE.init("vAAE", "Access simulated before preceding eviction");
            profNoViolAcc.init("noViolAcc", "Accesses without R/WAR/W violations");
            profNoViolEv.init("noViolEv",  "Evictions without AAE violations");

            parentStat->append(&profRAW);
            parentStat->append(&profWAR);
            parentStat->append(&profRAR);
            parentStat->append(&profWAW);
            parentStat->append(&profAAE);
            parentStat->append(&profNoViolAcc);
            parentStat->append(&profNoViolEv);
        }

        void update(uint32_t id, const MemReq* req) {
            T::update(id, req);

            bool read = (req->type == GETS);
            assert(read || req->type == GETX);
            uint64_t cycle = req->cycle;

            if (cycle < MAX(accTimes[id].read, accTimes[id].write)) { //violation
                //Now have to determine order
                bool readViol;
                if (cycle < MIN(accTimes[id].read, accTimes[id].write)) { //before both
                    readViol = (accTimes[id].read < accTimes[id].write); //read is closer
                } else if (cycle < accTimes[id].read) { //write, current access, read -> XAR viol
                    readViol = true;
                } else { //read, current access, write -> XAW viol
                    assert(cycle < accTimes[id].write);
                    readViol = false;
                }

                //Record
                read? (readViol? profRAR.inc() : profRAW.inc()) : (readViol? profWAR.inc() : profWAW.inc());

                //info("0x%lx viol read %d readViol %d cycles: %ld | r %ld w %ld", req->lineAddr, read, readViol, cycle, accTimes[id].read, accTimes[id].write);
            } else {
                profNoViolAcc.inc();
            }

            //Record
            if (read) accTimes[id].read  = MAX(accTimes[id].read,  req->cycle);
            else      accTimes[id].write = MAX(accTimes[id].write, req->cycle);

            T::update(id, req);
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } // sxj

        void startReplacement(const MemReq* req) {
            T::startReplacement(req);

            replCycle = req->cycle;
        }

        void replaced(uint32_t id) {
            T::replaced(id);

            if (replCycle < MAX(accTimes[id].read, accTimes[id].write)) {
                profAAE.inc();
            } else {
                profNoViolEv.inc();
            }

            //Reset --- update() will set correctly
            accTimes[id].read = 0;
            accTimes[id].write = 0;
        }
};

#endif  // REPL_POLICIES_H_
