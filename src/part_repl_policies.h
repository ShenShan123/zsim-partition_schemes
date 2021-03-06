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

#ifndef PART_REPL_POLICIES_H_
#define PART_REPL_POLICIES_H_

#include <sstream>
#include <stdint.h>
#include <iostream>
#include "event_queue.h"
#include "mtrand.h"
#include "partition_mapper.h"
#include "partitioner.h"
#include "repl_policies.h"

struct PartInfo {
    uint64_t size; //in lines
    uint64_t targetSize; //in lines

    Counter profHits;
    Counter profMisses;
    Counter profSelfEvictions; // from our same partition
    Counter profExtEvictions; // from other partitions (if too large, we're probably doing something wrong, e.g., too small an adjustment period)
};

class PartReplPolicy : public virtual ReplPolicy {
    protected:
        PartitionMonitor* monitor;
        PartMapper* mapper;

    public:
        PartReplPolicy(PartitionMonitor* _monitor, PartMapper* _mapper) : monitor(_monitor), mapper(_mapper) {}
        ~PartReplPolicy() { delete monitor; }

        virtual void setPartitionSizes(const uint32_t* sizes) = 0;

        PartitionMonitor* getMonitor() { return monitor; }

        const PartitionMonitor* getMonitor() const { return monitor; }
};

#define STRICT_WAY_PART 1 // control whether we do a strictly way partition, by shen
class WayPartReplPolicy : public PartReplPolicy, public LegacyReplPolicy {
    private:
        PartInfo* partInfo;
        uint32_t partitions;

        uint32_t totalSize;
        uint32_t waySize;
        uint32_t ways;

        struct WayPartInfo {
            Address addr; //FIXME: This is redundant due to the replacement policy interface
            uint64_t ts; //timestamp, >0 if in the cache, == 0 if line is empty
            uint32_t p;
        };

        WayPartInfo* array;

        uint32_t* wayPartIndex; //stores partition of each way

#if !STRICT_WAY_PART
        bool testMode;
#endif

        PAD();

        //Replacement process state (RW)
        int32_t bestId;
        uint32_t candIdx;
        uint32_t incomingLinePart; //to what partition does the incoming line belong?
        Address incomingLineAddr;

        //Globally incremented, but bears little significance per se
        uint64_t timestamp;

    public:
        WayPartReplPolicy(PartitionMonitor* _monitor, PartMapper* _mapper, uint64_t _lines, uint32_t _ways, bool _testMode)
                : PartReplPolicy(_monitor, _mapper), totalSize(_lines), ways(_ways)
        {
            partitions = mapper->getNumPartitions();
            waySize = totalSize/ways; // start with evenly sized partitions
            assert(waySize*ways == totalSize); //no partial ways...

            partInfo = gm_calloc<PartInfo>(partitions);
            for (uint32_t i = 0; i < partitions; i++) {
                partInfo[i].targetSize = 0;
                //Need placement new, these object have vptr
                new (&partInfo[i].profHits) Counter;
                new (&partInfo[i].profMisses) Counter;
                new (&partInfo[i].profSelfEvictions) Counter;
                new (&partInfo[i].profExtEvictions) Counter;
            }

            array = gm_calloc<WayPartInfo>(totalSize); //all have ts, p == 0...
            wayPartIndex = gm_calloc<uint32_t>(ways);

            for (uint32_t w = 0; w < ways; w++) {
                //Do initial way assignment, partitioner has no profiling info yet
                uint32_t p = w*partitions/ways; // in [0, ..., partitions-1] // if partition number > ways, then some of the cores cannot have their own parts.
                wayPartIndex[w] = p;
                partInfo[p].targetSize += waySize;
            }
#if STRICT_WAY_PART
            // strict way partition, by shen
            for (uint32_t i = 0; i < totalSize; ++i) {
                uint32_t way = i % ways;
                array[i].p = wayPartIndex[way];
                partInfo[wayPartIndex[way]].size++;
            }
#else
            partInfo[0].size = totalSize; // so partition 0 has all the lines
            testMode = _testMode;
#endif
            // end, strict way partition, by shen

            candIdx = 0;
            bestId = -1;
            timestamp = 1;
        }

        void initStats(AggregateStat* parentStat) {
            //AggregateStat* partsStat = new AggregateStat(true /*this is a regular aggregate, ONLY PARTITION STATS GO IN HERE*/);
            AggregateStat* partsStat = new AggregateStat(false); //don't make it a regular aggregate... it gets compacted in periodic stats and becomes useless!
            partsStat->init("part", "Partition stats");
            for (uint32_t p = 0; p < partitions; p++) {
                std::stringstream pss;
                pss << "part-" << p;
                AggregateStat* partStat = new AggregateStat();
                partStat->init(gm_strdup(pss.str().c_str()), "Partition stats");
                ProxyStat* pStat;
                pStat = new ProxyStat(); pStat->init("sz", "Actual size", &partInfo[p].size); partStat->append(pStat);
                pStat = new ProxyStat(); pStat->init("tgtSz", "Target size", &partInfo[p].targetSize); partStat->append(pStat);
                partInfo[p].profHits.init("hits", "Hits"); partStat->append(&partInfo[p].profHits);
                partInfo[p].profMisses.init("misses", "Misses"); partStat->append(&partInfo[p].profMisses);
                partInfo[p].profSelfEvictions.init("selfEvs", "Evictions caused by us"); partStat->append(&partInfo[p].profSelfEvictions);
                partInfo[p].profExtEvictions.init("extEvs", "Evictions caused by others"); partStat->append(&partInfo[p].profExtEvictions);

                partsStat->append(partStat);
            }
            parentStat->append(partsStat);
        }

        void update(uint32_t id, const MemReq* req) {
            WayPartInfo* e = &array[id];
            if (e->ts > 0) { //this is a hit update
                partInfo[e->p].profHits.inc();
            } else { //post-miss update, old line has been removed, this is empty
                uint32_t oldPart = e->p;
                uint32_t newPart = incomingLinePart;
                if (oldPart != newPart) {
                    partInfo[oldPart].size--;
                    partInfo[oldPart].profExtEvictions.inc();
                    partInfo[newPart].size++;
                } else {
                    partInfo[oldPart].profSelfEvictions.inc();
                }
                partInfo[newPart].profMisses.inc();
                e->p = newPart;
            }
            e->ts = timestamp++;

            //Update partitioner...
            monitor->access(e->p, e->addr);
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } //sxj

        void startReplacement(const MemReq* req) {
            assert(candIdx == 0);
            assert(bestId == -1);
            incomingLinePart = mapper->getPartition(*req);
            incomingLineAddr = req->lineAddr;
        }

        void recordCandidate(uint32_t id) {
            assert(candIdx < ways);
            WayPartInfo* c = &array[id]; //candidate info
            WayPartInfo* best = (bestId >= 0)? &array[bestId] : nullptr;
            uint32_t way = candIdx++;
#if STRICT_WAY_PART // by shen
            uint32_t sharedPartsInWay = partitions / ways;
            // This index is for the condition that serval partitions share one way: w0:[0,1] w1:[2,3] ...
            // So the wayPartIndex[0]:0 wayPartIndex[1]:2 ...
            uint32_t partIdx = sharedPartsInWay ? (incomingLinePart / sharedPartsInWay * sharedPartsInWay) : incomingLinePart;
            // we find the candidate from this shared partition
            if (wayPartIndex[way] == partIdx) {
                if (best == nullptr)
                    bestId = id;
                else if (c->ts < best->ts) 
                    bestId = id;
            }
            //info("incoming p %d, partIdx %d, cand part %d, wayPartIndex[%d] %d, size %ld", 
                //incomingLinePart, partIdx, c->p, way, wayPartIndex[way], partInfo[incomingLinePart].size);
#else // end, by shen
            //In test mode, this works as LRU
            if (testMode || wayPartIndex[way] == incomingLinePart) { //this is a way we can fill
                if (best == nullptr) {
                    bestId = id;
                } else {
                    //NOTE: This is actually not feasible without tagging. But what IS feasible is to stop updating the LRU position on new fills. We could kill this, and profile the differences.
                    if ( testMode || (c->p == incomingLinePart && best->p == incomingLinePart) ) {
                        if (c->ts < best->ts) bestId = id;
                    } else if (c->p == incomingLinePart && best->p != incomingLinePart) {
                        //c wins
                    } else if (c->p != incomingLinePart && best->p == incomingLinePart) {
                        //c loses
                        bestId = id;
                    } else { //none in our partition, this should be transient but at least enforce LRU
                        if (c->ts < best->ts) bestId = id;
                    }
                }
            }
#endif
        }

        uint32_t getBestCandidate() {
            // strict way partition, by shen
            assert(candIdx == ways);
            assert(bestId >= 0);
            return bestId;
        }

        void replaced(uint32_t id) {
            candIdx = 0;
            bestId = -1;
            array[id].ts = 0;
            array[id].addr = incomingLineAddr;
            //info("0x%lx", incomingLineAddr);
        }

    private:
        void setPartitionSizes(const uint32_t* waysPart) {
            uint32_t curWay = 0;
            for (uint32_t p = 0; p < partitions; p++) {
                //partInfo[p].targetSize = totalSize*waysPart[p]/ways;
#if UMON_INFO
                info("part %d assigned %d ways", p, waysPart[p]);
#endif
                for (uint32_t i = 0; i < waysPart[p]; i++) wayPartIndex[curWay++] = p;
            }
#if UMON_INFO
            for (uint32_t w = 0; w < ways; w++) info("wayPartIndex[%d] = %d", w, wayPartIndex[w]);
#endif
            assert(curWay == ways);
        }
};

class SetPartReplPolicy : public PartReplPolicy, public LegacyReplPolicy {
    private:
        PartInfo* partInfo;
        uint32_t partitions;

        uint32_t totalSize;
        uint32_t sets;
        uint32_t ways;

        struct SetPartInfo {
            Address addr; //FIXME: This is redundant due to the replacement policy interface
            uint64_t ts; //timestamp, >0 if in the cache, == 0 if line is empty
            uint32_t p;
        };

        SetPartInfo* array;

        uint32_t* setPartIndex; //stores partition of each way

        PAD();

        //Replacement process state (RW)
        int32_t bestId;
        uint32_t candIdx;
        uint32_t incomingLinePart; //to what partition does the incoming line belong?
        Address incomingLineAddr;
        uint64_t currSet; // the set that we currently operate

        //Globally incremented, but bears little significance per se
        uint64_t timestamp;
        HashFamily* hf;

    public:
        SetPartReplPolicy(PartitionMonitor* _monitor, PartMapper* _mapper, uint64_t _lines, uint32_t _ways, HashFamily* _hf)
                : PartReplPolicy(_monitor, _mapper), totalSize(_lines), ways(_ways), hf(_hf)
        {
            partitions = mapper->getNumPartitions();
            sets = totalSize/ways;
            currSet = sets;
            assert(partitions < sets); // make sure that the # of partitions is larger than # of sets

            partInfo = gm_calloc<PartInfo>(partitions);
            for (uint32_t i = 0; i < partitions; i++) {
                partInfo[i].targetSize = 0;
                //Need placement new, these object have vptr
                new (&partInfo[i].profHits) Counter;
                new (&partInfo[i].profMisses) Counter;
                new (&partInfo[i].profSelfEvictions) Counter;
                new (&partInfo[i].profExtEvictions) Counter;
            }

            array = gm_calloc<SetPartInfo>(totalSize); //all have ts, p == 0...
            setPartIndex = gm_calloc<uint32_t>(sets);
            uint32_t p = 0;
            for (uint32_t s = 0; s < sets; s++) {
                setPartIndex[s] = p++; // we assign the sets to each partition iteratively
                p = (p == partitions) ? 0 : p;
                //info("setPartIndex[%d] %d", s, setPartIndex[s]);
            }

            // strict set partition, by shen
            for (uint32_t i = 0; i < totalSize; ++i) {
                uint32_t set = i / ways;
                array[i].p = setPartIndex[set];
                partInfo[setPartIndex[set]].size++;
            }
            assert(partInfo[0].size == totalSize / partitions);

            candIdx = 0;
            bestId = -1;
            timestamp = 1;
        }

        inline uint64_t mapSet(uint32_t c, const Address lineAddr, const MemReq* req) {
            uint64_t set = hf->hash(c, lineAddr) & (sets - 1);
            set = set / partitions * partitions + mapper->getPartition(*req);
            currSet = set;
            return set;
        }

        void initStats(AggregateStat* parentStat) {
            //AggregateStat* partsStat = new AggregateStat(true /*this is a regular aggregate, ONLY PARTITION STATS GO IN HERE*/);
            AggregateStat* partsStat = new AggregateStat(false); //don't make it a regular aggregate... it gets compacted in periodic stats and becomes useless!
            partsStat->init("part", "Partition stats");
            for (uint32_t p = 0; p < partitions; p++) {
                std::stringstream pss;
                pss << "part-" << p;
                AggregateStat* partStat = new AggregateStat();
                partStat->init(gm_strdup(pss.str().c_str()), "Partition stats");
                ProxyStat* pStat;
                pStat = new ProxyStat(); pStat->init("sz", "Actual size", &partInfo[p].size); partStat->append(pStat);
                pStat = new ProxyStat(); pStat->init("tgtSz", "Target size", &partInfo[p].targetSize); partStat->append(pStat);
                partInfo[p].profHits.init("hits", "Hits"); partStat->append(&partInfo[p].profHits);
                partInfo[p].profMisses.init("misses", "Misses"); partStat->append(&partInfo[p].profMisses);
                partInfo[p].profSelfEvictions.init("selfEvs", "Evictions caused by us"); partStat->append(&partInfo[p].profSelfEvictions);
                partInfo[p].profExtEvictions.init("extEvs", "Evictions caused by others"); partStat->append(&partInfo[p].profExtEvictions);

                partsStat->append(partStat);
            }
            parentStat->append(partsStat);
        }

        void update(uint32_t id, const MemReq* req) {
            SetPartInfo* e = &array[id];
            if (e->ts > 0) { //this is a hit update
                partInfo[e->p].profHits.inc();
            } else { //post-miss update, old line has been removed, this is empty
                uint32_t oldPart = e->p;
                uint32_t newPart = incomingLinePart;
                if (oldPart != newPart) {
                    partInfo[oldPart].size--;
                    partInfo[oldPart].profExtEvictions.inc();
                    partInfo[newPart].size++;
                } else {
                    partInfo[oldPart].profSelfEvictions.inc();
                }
                partInfo[newPart].profMisses.inc();
                e->p = newPart;
            }
            e->ts = timestamp++;

            //Update partitioner...
            monitor->access(e->p, e->addr);
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } //sxj

        void startReplacement(const MemReq* req) {
            assert(candIdx == 0);
            assert(bestId == -1);
            incomingLinePart = mapper->getPartition(*req);
            incomingLineAddr = req->lineAddr;
            //info("set %ld, currSet %ld, setPartIndex %d, incomingLinePart %d, size %ld", mapSet(0, incomingLineAddr, req), currSet, setPartIndex[currSet], (int)incomingLinePart, partInfo[incomingLinePart].size);
            assert(setPartIndex[currSet] == incomingLinePart);
        }

        void recordCandidate(uint32_t id) {
            assert(candIdx++ < ways);
            SetPartInfo& e = array[id];
            assert(e.p == setPartIndex[currSet]);

            if (bestId == -1)
                bestId = id;
            else if (e.ts < array[bestId].ts) {
                bestId = id;
            }
        }

        uint32_t getBestCandidate() {
            // strict way partition, by shen
            assert(candIdx == ways);
            assert(bestId >= 0);
            return bestId;
        }

        void replaced(uint32_t id) {
            candIdx = 0;
            bestId = -1;
            array[id].ts = 0;
            array[id].addr = incomingLineAddr;
            //info("0x%lx", incomingLineAddr);
        }

    private:
        void setPartitionSizes(const uint32_t* waysPart) {}
};

#define VANTAGE_8BIT_BTS 1 //1 for 8-bit coarse-grain timestamps, 0 for 64-bit coarse-grain (no wrap-arounds)

/* Vantage replacement policy. Please refer to our ISCA 2011 paper for implementation details.
 */
class VantageReplPolicy : public PartReplPolicy, public LegacyReplPolicy {
    private:
        /* NOTE: This implementation uses 64-bit coarse-grain TSs for simplicity. You have a choice of constraining
         * these to work 8-bit timestamps by setting VANTAGE_8BIT_BTS to 1. Note that this code still has remnants of
         * the 64-bit global fine-grain timestamps used to simulate perfect LRU. They are not using for anything but profiling.
         */
        uint32_t partitions;
        uint32_t totalSize;
        uint32_t assoc;

        struct VantagePartInfo : public PartInfo {
            uint64_t curBts; //per-partition coarse-grain timestamp (CurrentTS in paper)
            uint32_t curBtsHits; //hits on current timestamp (AccessCounter in paper)

            uint64_t setpointBts; // setpoint coarse-grain timestamp (SetpointTS in paper)
            uint64_t setpointAdjs; // setpoint adjustments so far, just for profiling purposes

            uint32_t curIntervalIns; // insertions in current interval. Not currently used.
            uint32_t curIntervalDems; // CandsDemoted in paper
            uint32_t curIntervalCands; // CandsSeen in paper

            uint64_t extendedSize;

            uint64_t longTermTargetSize; //in lines

            Counter profDemotions;
            Counter profEvictions;
            Counter profSizeCycles;
            Counter profExtendedSizeCycles;
        };

        VantagePartInfo* partInfo;

        struct LineInfo {
            Address addr; //FIXME: This is redundant due to the replacement policy interface
            uint64_t ts; //timestamp, >0 if in the cache, == 0 if line is empty (little significance otherwise)
            uint64_t bts; //coarse-grain per-partition timestamp
            uint32_t p; //partition ID
            uint32_t op; //original partition id: same as partition id when in partition, but does not change when moved to FFA (unmanaged region)
        };

        LineInfo* array;

        Counter profPromotions;
        Counter profUpdateCycles;

        //Repl process stuff
        uint32_t* candList;
        uint32_t candIdx;
        Address incomingLineAddr;

        //Globally incremented, but bears little significance per se
        uint64_t timestamp;

        double partPortion; //how much of the cache do we devote to the partition's target sizes?
        double partSlack; //how much the aperture curve reacts to "cushion" the load. partSlack+targetSize sets aperture to 1.0
        double maxAperture; //Maximum aperture allowed in each partition, must be < 1.0
        uint32_t partGranularity; //number of partitions that UMON/LookaheadPartitioner expects

        uint64_t lastUpdateCycle; //for cumulative size counter updates; could be made event-driven

        MTRand rng;
        bool smoothTransients; //if set, keeps all growing partitions at targetSz = actualSz + 1 until they reach their actual target; takes space away slowly from the shrinking partitions instead of aggressively demoting them to the unmanaged region, which turns the whole thing into a shared cache if transients are frequent

    public:
        VantageReplPolicy(PartitionMonitor* _monitor, PartMapper* _mapper, uint64_t _lines,  uint32_t _assoc, uint32_t partPortionPct,
                          uint32_t partSlackPct, uint32_t maxAperturePct, uint32_t _partGranularity, bool _smoothTransients)
                : PartReplPolicy(_monitor, _mapper), totalSize(_lines), assoc(_assoc), rng(0xABCDE563F), smoothTransients(_smoothTransients)
        {
            partitions = mapper->getNumPartitions();

            assert(partPortionPct <= 100);
            assert(partSlackPct <= 100);
            assert(maxAperturePct <= 100);

            partPortion = ((double)partPortionPct)/100.0;
            partSlack = ((double)partSlackPct)/100.0;
            maxAperture = ((double)maxAperturePct)/100.0;
            partGranularity = _partGranularity;  // NOTE: partitioning at too fine granularity (+1K buckets) overwhelms the lookahead partitioner

            uint32_t targetManagedSize = (uint32_t)(((double)totalSize)*partPortion);

            partInfo = gm_calloc<VantagePartInfo>(partitions+1);  // last one is unmanaged region

            for (uint32_t i = 0; i <= partitions; i++) {
                partInfo[i].targetSize = targetManagedSize/partitions;
                partInfo[i].longTermTargetSize = partInfo[i].targetSize;
                partInfo[i].extendedSize = 0;

                //Need placement new, these objects have vptr
                new (&partInfo[i].profHits) Counter;
                new (&partInfo[i].profMisses) Counter;
                new (&partInfo[i].profSelfEvictions) Counter;
                new (&partInfo[i].profExtEvictions) Counter;
                new (&partInfo[i].profDemotions) Counter;
                new (&partInfo[i].profEvictions) Counter;
                new (&partInfo[i].profSizeCycles) Counter;
                new (&partInfo[i].profExtendedSizeCycles) Counter;
            }

            //unmanaged region should not use these
            partInfo[partitions].targetSize = 0;
            partInfo[partitions].longTermTargetSize = 0;

            array = gm_calloc<LineInfo>(totalSize);

            //Initially, assign all the lines to the unmanaged region
            partInfo[partitions].size = totalSize;
            partInfo[partitions].extendedSize = totalSize;
            for (uint32_t i = 0; i < totalSize; i++) {
                array[i].p = partitions;
                array[i].op = partitions;
            }

            candList = gm_calloc<uint32_t>(assoc);
            candIdx = 0;
            timestamp = 1;

            lastUpdateCycle = 0;

            info("Vantage RP: %d partitions, managed portion %f Amax %f slack %f", partitions, partPortion, maxAperture, partSlack);
        }

        void initStats(AggregateStat* parentStat) {
            AggregateStat* rpStat = new AggregateStat();
            rpStat->init("part", "Vantage replacement policy stats");
            ProxyStat* pStat;
            profPromotions.init("ffaProms", "Promotions from unmanaged region"); rpStat->append(&profPromotions);
            profUpdateCycles.init("updCycles", "Cycles of updates experienced on size-cycle counters"); rpStat->append(&profUpdateCycles);
            for (uint32_t p = 0; p <= partitions; p++) {
                std::stringstream pss;
                pss << "part-" << p;
                AggregateStat* partStat = new AggregateStat();
                partStat->init(gm_strdup(pss.str().c_str()), "Partition stats");

                pStat = new ProxyStat(); pStat->init("sz", "Actual size", &partInfo[p].size); partStat->append(pStat);
                pStat = new ProxyStat(); pStat->init("xSz", "Extended actual size, including lines currently demoted to FFA", &partInfo[p].extendedSize); partStat->append(pStat);
                //NOTE: To avoid breaking scripts, I've changed tgtSz to track longTermTargetSize
                //FIXME: Code and stats should be named similarly
                pStat = new ProxyStat(); pStat->init("tgtSz", "Target size", &partInfo[p].longTermTargetSize); partStat->append(pStat);
                pStat = new ProxyStat(); pStat->init("stTgtSz", "Short-term target size (used with smoothedTransients)", &partInfo[p].targetSize); partStat->append(pStat);
                partInfo[p].profHits.init("hits", "Hits"); partStat->append(&partInfo[p].profHits);
                partInfo[p].profMisses.init("misses", "Misses"); partStat->append(&partInfo[p].profMisses);
                //Vantage does not do evictions directly, these do not make sense and are not used
                //partInfo[p].profSelfEvictions.init("selfEvs", "Evictions caused by us"); partStat->append(&partInfo[p].profSelfEvictions);
                //partInfo[p].profExtEvictions.init("extEvs", "Evictions caused by others"); partStat->append(&partInfo[p].profExtEvictions);
                partInfo[p].profDemotions.init("dems", "Demotions"); partStat->append(&partInfo[p].profDemotions);
                partInfo[p].profEvictions.init("evs", "Evictions"); partStat->append(&partInfo[p].profEvictions);
                partInfo[p].profSizeCycles.init("szCycles", "Cumulative per-cycle sum of sz"); partStat->append(&partInfo[p].profSizeCycles);
                partInfo[p].profExtendedSizeCycles.init("xSzCycles", "Cumulative per-cycle sum of xSz"); partStat->append(&partInfo[p].profExtendedSizeCycles);

                rpStat->append(partStat);
            }
            parentStat->append(rpStat);
        }

        void update(uint32_t id, const MemReq* req) {
            if (unlikely(zinfo->globPhaseCycles > lastUpdateCycle)) {
                //Update size-cycle counter stats
                uint64_t diff = zinfo->globPhaseCycles - lastUpdateCycle;
                for (uint32_t p = 0; p <= partitions; p++) {
                    partInfo[p].profSizeCycles.inc(diff*partInfo[p].size);
                    partInfo[p].profExtendedSizeCycles.inc(diff*partInfo[p].extendedSize);
                }
                profUpdateCycles.inc(diff);
                lastUpdateCycle = zinfo->globPhaseCycles;
            }

            LineInfo* e = &array[id];
            if (e->ts > 0) {
                if (e->p == partitions) { //this is an unmanaged region promotion
                    e->p = mapper->getPartition(*req);
                    profPromotions.inc();
                    partInfo[e->p].curIntervalIns++;
                    partInfo[e->p].size++;
                    partInfo[partitions].size--;
                }
                e->ts = timestamp++;
                partInfo[e->p].profHits.inc();
            } else { //post-miss update, old one has been removed, this is empty
                e->ts = timestamp++;
                partInfo[e->p].size--;
                partInfo[e->p].profEvictions.inc();
                partInfo[e->op].extendedSize--;
                e->p = mapper->getPartition(*req);
                e->op = e->p;
                partInfo[e->p].curIntervalIns++;
                partInfo[e->p].size++;
                partInfo[e->op].extendedSize++;
                partInfo[e->p].profMisses.inc();

                if (partInfo[e->p].targetSize < partInfo[e->p].longTermTargetSize) {
                    assert(smoothTransients);
                    partInfo[e->p].targetSize++;
                    takeOneLine();
                }
            }

            //Profile the access
            monitor->access(e->p, e->addr);

            //Adjust coarse-grain timestamp
            e->bts = partInfo[e->p].curBts;
            if (++partInfo[e->p].curBtsHits >= (uint32_t) partInfo[e->p].size/16) {
                partInfo[e->p].curBts++;
                partInfo[e->p].setpointBts++;
                partInfo[e->p].curBtsHits = 0;
            }
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } //sxj

        void startReplacement(const MemReq* req) {
            incomingLineAddr = req->lineAddr;
        }

        void recordCandidate(uint32_t id) {
            assert(candIdx < assoc);
            candList[candIdx++] = id;
        }

        uint32_t getBestCandidate() {
            assert(candIdx > 0);
            assert(candIdx <= assoc);

            //Demote all lines below their setpoints
            for (uint32_t i = 0; i < candIdx; i++) {
                LineInfo* e = &array[candList[i]];
                if (e->ts == 0) continue; //empty, bypass

                uint32_t p = e->p;
                if (p == partitions) continue; //bypass unmanaged region entries

                uint32_t size = partInfo[p].size;

                if (size <= partInfo[p].targetSize) continue; //bypass partitions below target

#if VANTAGE_8BIT_BTS
                //Must do mod 256 arithmetic. This will do generally worse because of wrap-arounds, but wrapping around is pretty rare
                //TODO: Doing things this way, we can profile the difference between this and using larger coarse-grain timestamps
                if (((partInfo[p].curBts - e->bts) % 256) /*8-bit distance to current TS*/ >= ((partInfo[p].curBts - partInfo[p].setpointBts) % 256)) {
#else
                if (e->bts <= partInfo[p].setpointBts) {
#endif
                    // Demote!
                    // Out of p
                    partInfo[p].profDemotions.inc();
                    partInfo[p].size--;

                    // Into unmanaged
                    e->p = partitions;
                    partInfo[partitions].size++;

                    partInfo[p].curIntervalDems++;

                    //Note extended size and op not affected
                }

                partInfo[p].curIntervalCands++;

                // See if we need interval change
                if (/*partInfo[p].curIntervalDems >= 16 || partInfo[p].curIntervalIns >= 16 ||*/ partInfo[p].curIntervalCands >= 256) {
                    double maxSz = partInfo[p].targetSize*(1.0 + partSlack);
                    double curSz = partInfo[p].size;
                    double aperture = 0.0;

                    // Feedback-based aperture control
                    // TODO: Copy over the demotion thresholds lookup table code from the ISCA paper code, or quantize this.
                    // This is doing finer-grain demotions, but requires a bit more math.
                    if (curSz >= maxSz) {
                        aperture = maxAperture;
                    } else {
                        double slope = (maxAperture)/(maxSz - partInfo[p].targetSize);
                        assert(slope > 0.0);
                        aperture = slope*(curSz - partInfo[p].targetSize);
                    }

                    if (aperture > 0.0) {
/*
                        info ("part %d setpoint adjust, curSz %f tgtSz %ld maxSz %f aperture %f curBts %ld setpointBts %ld interval cands %d ins %d dems %d cpt %f",
                            p, curSz, partInfo[p].targetSize, maxSz, aperture, partInfo[p].curBts, partInfo[p].setpointBts, partInfo[p].curIntervalCands,\
                            partInfo[p].curIntervalIns, partInfo[p].curIntervalDems, partInfo[p].curIntervalCands*aperture);
*/

                        int32_t shrink = partInfo[p].curIntervalDems;
                        if (shrink < aperture*partInfo[p].curIntervalCands) {
                            //info ("increasing setpoint");
                            if (partInfo[p].setpointBts < partInfo[p].curBts) partInfo[p].setpointBts++;
                        } else if (shrink > aperture*partInfo[p].curIntervalCands) {
                            //info ("decreasing setpoint");
#if VANTAGE_8BIT_BTS
                            //Never get the setpoint to go 256 positions behind the current timestamp
                            if ((partInfo[p].curBts - partInfo[p].setpointBts) < 255) partInfo[p].setpointBts--;
#else
                            if (partInfo[p].setpointBts > 0) partInfo[p].setpointBts--;
#endif
                        } else {
                            //info ("keeping setpoint");
                        }
                    }

                    //info("part %d post setpointBts %ld", p, partInfo[p].setpointBts);

                    partInfo[p].curIntervalCands = 0;
                    partInfo[p].curIntervalIns = 0;
                    partInfo[p].curIntervalDems = 0;
                    partInfo[p].setpointAdjs++;
                }
            } //for

            //Get best candidate for eviction
            int32_t bestId = candList[0];

            for (uint32_t i = 0; i < candIdx; i++) { //note we include 0; 0 compares with itself, see shortcut to understand why
                uint32_t id = candList[i];
                LineInfo* e = &array[id];
                LineInfo* best = &array[bestId];

                if (e->ts == 0) {
                    //shortcut for empty positions
                    bestId = id;
                    break;
                }

                uint32_t p = e->p;

                if (p == partitions && best->p != partitions) { //prioritize umgd
                    bestId = id;
                } else if (p == partitions && best->p == partitions) {
                    if (e->ts < best->ts) bestId = id;
                } else if (p != partitions && best->p == partitions) {
                    //best wins, prioritize unmanaged
                } else {
                    assert(p != partitions && best->p != partitions);
                    //Just do LRU; with correctly-sized partitions, this is VERY rare
                    //NOTE: If we were to study really small unmanaged regions, we can always get fancier and prioritize by aperture, bts, etc.
                    if (e->ts < best->ts) bestId = id;
                }
            }
            assert(bestId >= 0 && (uint32_t)bestId < totalSize);
            return bestId;
        }

        void replaced(uint32_t id) {
            candIdx = 0; //reset
            LineInfo* e = &array[id];
            e->ts = 0;
            e->bts = 0;
            e->addr = incomingLineAddr;
        }

    private:
        void setPartitionSizes(const uint32_t* sizes) {
            uint32_t s[partitions];
            uint32_t usedSize = 0;
            uint32_t linesToTakeAway = 0;
            for (uint32_t p = 0; p < partitions; p++) {
                s[p] = totalSize*sizes[p]/partGranularity;// according to the init.cpp, for vantage, the partGranularity is 256, thus the num of UMON bins is way num
#if UMON_INFO
                info("part %d, %ld -> %d lines (now it's %ld lines) [cur %ld/%ld set %ld/%ld setAdjs %ld] total size: %d granularity %d, alloc size %d;", p, partInfo[p].targetSize, s[p],
                        partInfo[p].size, partInfo[p].curBts, partInfo[p].curBts % 256, partInfo[p].setpointBts, partInfo[p].setpointBts % 256, partInfo[p].setpointAdjs, totalSize, partGranularity, sizes[p]);
#endif

                if (smoothTransients) {
                    partInfo[p].longTermTargetSize = s[p];
                    if (s[p] > partInfo[p].targetSize) { //growing
                        uint32_t newTarget = MAX(partInfo[p].targetSize, MIN(partInfo[p].longTermTargetSize, partInfo[p].size+1)); //always in [target,longTermTarget]
                        linesToTakeAway += newTarget - partInfo[p].targetSize;
                        partInfo[p].targetSize = newTarget;
                    }
                    // where is the else ??
                } else {
                    partInfo[p].targetSize = s[p];
                    partInfo[p].longTermTargetSize = s[p];
                }
                usedSize += s[p];
            }

            while (linesToTakeAway--) takeOneLine();
#if UMON_INFO
            info("%d lines assigned, %d unmanaged", usedSize, totalSize - usedSize);
#endif
        }

        void takeOneLine() {
            assert(smoothTransients);
            uint32_t linesLeft = 0;
            //NOTE: This is a fairly inefficient implementation, but we can do it cheaply in hardware
            //Take away proportionally to difference between actual and long-term target
            for (uint32_t p = 0; p < partitions; p++) {
                int32_t left = partInfo[p].targetSize - partInfo[p].longTermTargetSize;
                linesLeft += MAX(left, 0);
            }
            assert(linesLeft > 0);
            uint32_t l = rng.randInt(linesLeft-1); //[0, linesLeft-1]
            uint32_t curLines = 0;
            for (uint32_t p = 0; p < partitions; p++) {
                int32_t left = partInfo[p].targetSize - partInfo[p].longTermTargetSize;
                curLines += MAX(left, 0);
                if (left > 0 && l < curLines) {
                    partInfo[p].targetSize--;
                    return;
                }
            }
            panic("Could not find any partition to take away space from???");
        }
};

/* Futility Scaling scheme, the details are in MICRO 2014 */
class FutilityScaling : public PartReplPolicy, public LegacyReplPolicy {
    private:
        struct FsPartInfo : public PartInfo {
            uint64_t curBts; //per-partition coarse-grain timestamp (CurrentTS in paper)
            uint64_t curBtsHits; //hits on current timestamp (AccessCounter in paper)

            uint32_t insertions; // the inserted lines in this part
            uint32_t evictions;

            uint64_t scalingShiftWidth; // the delta scaling factor is set to 2, thus we can adjust scaling factor with a shifting op.

            Counter profInsertions;
            Counter profEvictions;
            Counter profSizeCycles;
        };
        
        FsPartInfo* partInfo;
        uint32_t partitions;

        uint32_t totalSize;
        uint32_t assoc;
        uint32_t partGranularity; //number of partitions that UMON/LookaheadPartitioner expects

        struct LineInfo {
            Address addr; //FIXME: This is redundant due to the replacement policy interface
            uint64_t ts; //timestamp, >0 if in the cache, == 0 if line is empty (little significance otherwise)
            uint64_t bts; //coarse-grain per-partition timestamp
            uint32_t p; //partition ID
        };

        LineInfo* array;

        Counter profUpdateCycles;

        //Replacement process state (RW)
        int32_t bestId;
        //Repl process stuff
        uint32_t* candList;
        uint32_t candIdx;
        uint32_t incomingLinePart; //to what partition does the incoming line belong?
        Address incomingLineAddr;

        //Globally incremented, but bears little significance per se
        uint64_t timestamp; // global timestamp
        uint32_t intervalLength;

        uint64_t lastUpdateCycle; //for cumulative size counter updates; could be made event-driven
        uint64_t maxShiftWidth;

    public:
        // the monitor for Futility Scaling scheme is UCP for default
        FutilityScaling(PartitionMonitor* _monitor, PartMapper* _mapper, uint64_t _lines,  uint32_t _assoc, uint32_t _partGranularity)
                : PartReplPolicy(_monitor, _mapper), totalSize(_lines), assoc(_assoc)
        {
            partitions = mapper->getNumPartitions();

            partGranularity = _partGranularity;  // NOTE: partitioning at too fine granularity (+1K buckets) overwhelms the lookahead partitioner
            partInfo = gm_calloc<FsPartInfo>(partitions);  // last one is unmanaged region

            for (uint32_t i = 0; i < partitions; i++) {
                partInfo[i].scalingShiftWidth = 4; // maximum scaling shift bit width is 7, so we set the medium value at the beginning
                // we first average out the total size to each partition
                partInfo[i].targetSize = totalSize / partitions;

                //Need placement new, these objects have vptr
                new (&partInfo[i].profHits) Counter;
                new (&partInfo[i].profMisses) Counter;
                new (&partInfo[i].profSelfEvictions) Counter;
                new (&partInfo[i].profExtEvictions) Counter;
                new (&partInfo[i].profInsertions) Counter;
                new (&partInfo[i].profEvictions) Counter;
                new (&partInfo[i].profSizeCycles) Counter;
            }

            array = gm_calloc<LineInfo>(totalSize);
            assert(totalSize >= partitions);

            for (uint32_t i = 0; i < totalSize; i++) {
                array[i].p = 0; // just for now, we set all lines to the first partition
            }
            partInfo[0].size = totalSize; // change the actual size

            bestId = -1;
            candList = gm_calloc<uint32_t>(assoc);
            candIdx = 0;

            timestamp = 1;
            intervalLength = 16; // the interval length l is set to 16 according to the MICRO14 paper

            lastUpdateCycle = 0;
            maxShiftWidth = 7; // according to the MICRO14 paper, the maximum scaling factor is 128 = 2^7

            info("Futility Scaling RP: %d partitions, partition granularity %d", partitions, partGranularity);
        }

        void initStats(AggregateStat* parentStat) {
            AggregateStat* rpStat = new AggregateStat();
            rpStat->init("part", "Futility Scaling replacement policy stats");
            ProxyStat* pStat;
            //profUpdateCycles.init("updCycles", "Cycles of updates experienced on size-cycle counters"); rpStat->append(&profUpdateCycles);
            for (uint32_t p = 0; p < partitions; p++) {
                std::stringstream pss;
                pss << "part-" << p;
                AggregateStat* partStat = new AggregateStat();
                partStat->init(gm_strdup(pss.str().c_str()), "Partition stats");

                pStat = new ProxyStat(); pStat->init("sz", "Actual size", &partInfo[p].size); partStat->append(pStat);
                //FIXME: Code and stats should be named similarly
                pStat = new ProxyStat(); pStat->init("tgtSz", "Target size", &partInfo[p].targetSize); partStat->append(pStat);
                partInfo[p].profHits.init("hits", "Hits"); partStat->append(&partInfo[p].profHits);
                partInfo[p].profMisses.init("misses", "Misses"); partStat->append(&partInfo[p].profMisses);
                partInfo[p].profSelfEvictions.init("selfEvs", "Evictions caused by us"); partStat->append(&partInfo[p].profSelfEvictions);
                partInfo[p].profExtEvictions.init("extEvs", "Evictions caused by others"); partStat->append(&partInfo[p].profExtEvictions);
                partInfo[p].profInsertions.init("ins", "Number of insertion"); partStat->append(&partInfo[p].profInsertions);
                partInfo[p].profEvictions.init("evs", "Number of Evictions"); partStat->append(&partInfo[p].profEvictions);
                pStat = new ProxyStat(); pStat->init("shiftW", "Scaling Shift Width", &partInfo[p].scalingShiftWidth); partStat->append(pStat);
                partInfo[p].profSizeCycles.init("szCycles", "Cumulative per-cycle sum of sz"); partStat->append(&partInfo[p].profSizeCycles);
                //partInfo[p].profExtendedSizeCycles.init("xSzCycles", "Cumulative per-cycle sum of xSz"); partStat->append(&partInfo[p].profExtendedSizeCycles);

                rpStat->append(partStat);
            }
            parentStat->append(rpStat);
        }

        void update(uint32_t id, const MemReq* req) {
            if (unlikely(zinfo->globPhaseCycles > lastUpdateCycle)) {
                //Update size-cycle counter stats
                uint64_t diff = zinfo->globPhaseCycles - lastUpdateCycle;
                for (uint32_t p = 0; p < partitions; p++) {
                    partInfo[p].profSizeCycles.inc(diff*partInfo[p].size);
                }

                profUpdateCycles.inc(diff);
                lastUpdateCycle = zinfo->globPhaseCycles;
            }

            LineInfo* e = &array[id];
            assert(e->p < partitions);
            if (e->ts > 0) {
                e->ts = timestamp++;
                partInfo[e->p].profHits.inc();
            } else { //post-miss update, old one has been removed, this is empty
                e->ts = timestamp++;
                partInfo[e->p].size--;
                partInfo[e->p].profEvictions.inc();
                partInfo[e->p].profSelfEvictions.inc(e->p == incomingLinePart);
                partInfo[e->p].profExtEvictions.inc(e->p != incomingLinePart);
                //zinfo("FS update: partition %d, number of evictions %d", (int)e->p, (int)partInfo[e->p].profEvictions.get());
                partInfo[e->p].evictions++;

                // for the new part
                assert(incomingLinePart < partitions);
                e->p = incomingLinePart;
                partInfo[e->p].size++;
                partInfo[e->p].profInsertions.inc();
                partInfo[e->p].insertions++;
                partInfo[e->p].profMisses.inc();
                //info("FS update: partition %d, number of insertions %d", (int)e->p, (int)partInfo[e->p].profInsertions.get());
            }

            //Profile the access
            monitor->access(e->p, e->addr);

            //Adjust coarse-grain timestamp
            e->bts = partInfo[e->p].curBts;

            if (++partInfo[e->p].curBtsHits >= (uint32_t) partInfo[e->p].size / 16) {
                partInfo[e->p].curBts++;
                partInfo[e->p].curBtsHits = 0;
            }
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } //sxj

        void startReplacement(const MemReq* req) {
            assert(candIdx == 0);
            assert(bestId == -1);
            incomingLinePart = mapper->getPartition(*req);
            incomingLineAddr = req->lineAddr;
        }

        void recordCandidate(uint32_t id) {
            assert(candIdx < assoc);
            candList[candIdx++] = id;
        }

        uint32_t getBestCandidate() {
            assert(candIdx > 0);
            assert(candIdx <= assoc);

            // update the scaling factor for all partitions
            for (uint32_t i = 0; i < partitions; i++) {
                FsPartInfo & part = partInfo[i];
                if (part.evictions >= intervalLength || part.insertions >= intervalLength) {
                    if (part.insertions >= part.evictions && part.size > part.targetSize) {
                        part.scalingShiftWidth += (part.scalingShiftWidth < maxShiftWidth); // need a scaling factor!!
                    }
                    else if (part.insertions < part.evictions && part.size < part.targetSize) {
                        part.scalingShiftWidth -= (part.scalingShiftWidth > 0);
                    }

                    part.evictions = 0;
                    part.insertions = 0;
                }
            }

            uint32_t maxScaledFactor = 0;

            //Demote all lines below their setpoints
            for (uint32_t i = 0; i < candIdx; i++) {
                LineInfo* e = &array[candList[i]];

                if (e->ts == 0) {
                    //shortcut for empty positions
                    bestId = candList[i];
                    break;
                }

                uint32_t factor = (partInfo[e->p].curBts + 256 - e->bts) % 256;
                uint32_t scaledFactor = factor << partInfo[e->p].scalingShiftWidth;
                
                if (scaledFactor > maxScaledFactor) {
                    maxScaledFactor = scaledFactor;
                    bestId = candList[i];
                }
                // we break the tie by comparing their timestamp
                else if (scaledFactor == maxScaledFactor) {
                    bestId = e->ts < array[bestId].ts ? candList[i] : bestId;
                }
            }
            
            assert(bestId >= 0 && (uint32_t)bestId < totalSize);
            return bestId;
        }

        void replaced(uint32_t id) {
            candIdx = 0; //reset
            bestId = -1;
            LineInfo* e = &array[id];
            e->ts = 0;
            e->bts = 0;
            e->addr = incomingLineAddr;
        }

    private:
        void setPartitionSizes(const uint32_t* sizes) {
            uint32_t s[partitions];

            for (uint32_t p = 0; p < partitions; p++) {
                assert(sizes[p] < partGranularity && sizes[p]); // the minimum size is 1, see the init.cpp

                s[p] = totalSize * sizes[p] / partGranularity;// according to the init.cpp, for vantage, the partGranularity is 256, thus the num of UMON bins is way num
#if UMON_INFO
                info("part %d, %ld -> %d lines (now it's %ld lines) [cur %ld/%ld];total size: %d, granularity %d, alloc size %d;", p, partInfo[p].targetSize, s[p],
                        partInfo[p].size, partInfo[p].curBts, partInfo[p].curBts % 256, totalSize, partGranularity, sizes[p]);
#endif
                // for FS partitioning, it doesn't need smoothtransient since it is replacement policy-based scheme
                partInfo[p].targetSize = s[p];
            }
        }
};

/* Futility Scaling scheme, the details are in ISCA 2012 */
class PriSM : public PartReplPolicy, public LegacyReplPolicy {
    private:
        struct PrPartInfo : public PartInfo {
            uint32_t interMisses; // Mi, the misses in this interval
            uint32_t interBeginSize; // Ci, the size at the beginning of a interval
            int32_t evictProb; // Ei * W, eviction probability multiplied by intervalLength

            Counter profInsertions;
            Counter profEvictions;
            Counter profSizeCycles;
        };
        
        PrPartInfo* partInfo;
        uint32_t partitions;

        uint32_t totalSize;
        uint32_t assoc;
        uint32_t partGranularity; //number of partitions that UMON/LookaheadPartitioner expects

        struct LineInfo {
            Address addr; //FIXME: This is redundant due to the replacement policy interface
            uint64_t ts; //timestamp, >0 if in the cache, == 0 if line is empty (little significance otherwise)
            uint32_t p; //partition ID
        };

        LineInfo* array;

        Counter profUpdateCycles;
        Counter profNoCand; // the occurances that the best partition with no candidates

        uint32_t bestPart; // the best partition ID
        //Replacement process state (RW)
        int32_t bestId;
        //Repl process stuff
        uint32_t* candList;
        uint32_t candIdx;
        uint32_t incomingLinePart; //to what partition does the incoming line belong?
        Address incomingLineAddr;

        //Globally incremented, but bears little significance per se
        uint64_t timestamp; // global timestamp
        uint32_t intervalLength; // W in the paper, i.e. the total misses in a interval
        uint32_t interMissCounter;

        uint64_t lastUpdateCycle; //for cumulative size counter updates; could be made event-driven

    public:
        // the monitor for Futility Scaling scheme is UCP for default
        PriSM(PartitionMonitor* _monitor, PartMapper* _mapper, uint64_t _lines,  uint32_t _assoc, uint32_t _partGranularity)
                : PartReplPolicy(_monitor, _mapper), totalSize(_lines), assoc(_assoc)
        {
            partitions = mapper->getNumPartitions();

            partGranularity = _partGranularity;  // NOTE: partitioning at too fine granularity (+1K buckets) overwhelms the lookahead partitioner
            partInfo = gm_calloc<PrPartInfo>(partitions + 1);

            for (uint32_t i = 0; i <= partitions; i++) {
                // we first average out the total size to each partition
                partInfo[i].targetSize = totalSize / partitions;

                //Need placement new, these objects have vptr
                new (&partInfo[i].profHits) Counter;
                new (&partInfo[i].profMisses) Counter;
                new (&partInfo[i].profSelfEvictions) Counter;
                new (&partInfo[i].profExtEvictions) Counter;
                new (&partInfo[i].profInsertions) Counter;
                new (&partInfo[i].profEvictions) Counter;
                new (&partInfo[i].profSizeCycles) Counter;
            }
            // for the unmanagement part, this part is only used at the simulation beginning
            partInfo[partitions].targetSize = 0;
            partInfo[partitions].size = totalSize;

            array = gm_calloc<LineInfo>(totalSize);
            assert(totalSize >= partitions);

            for (uint32_t i = 0; i < totalSize; i++)
                array[i].p = partitions; // partition idx = partitions indicates the line doesn't belongs to any partition
            
            bestPart = partitions;
            bestId = -1;
            candList = gm_calloc<uint32_t>(assoc);
            candIdx = 0;

            timestamp = 1;
            intervalLength = 128 * 1024; // the interval length W is set to 128K
            interMissCounter = 0;

            lastUpdateCycle = 0;

            info("PriSM RP: %d partitions, partition granularity %d", partitions, intervalLength);
        }

        void initStats(AggregateStat* parentStat) {
            AggregateStat* rpStat = new AggregateStat();
            rpStat->init("part", "PriSM replacement policy stats");
            ProxyStat* pStat;
            profNoCand.init("noCand", "the best partition with no candidates"); rpStat->append(&profNoCand);
            //profUpdateCycles.init("updCycles", "Cycles of updates experienced on size-cycle counters"); rpStat->append(&profUpdateCycles);
            for (uint32_t p = 0; p <= partitions; p++) {
                std::stringstream pss;
                pss << "part-" << p;
                AggregateStat* partStat = new AggregateStat();
                partStat->init(gm_strdup(pss.str().c_str()), "Partition stats");

                pStat = new ProxyStat(); pStat->init("sz", "Actual size", &partInfo[p].size); partStat->append(pStat);
                //FIXME: Code and stats should be named similarly
                pStat = new ProxyStat(); pStat->init("tgtSz", "Target size", &partInfo[p].targetSize); partStat->append(pStat);
                partInfo[p].profHits.init("hits", "Hits"); partStat->append(&partInfo[p].profHits);
                partInfo[p].profMisses.init("misses", "Misses"); partStat->append(&partInfo[p].profMisses);
                partInfo[p].profSelfEvictions.init("selfEvs", "Evictions caused by us"); partStat->append(&partInfo[p].profSelfEvictions);
                partInfo[p].profExtEvictions.init("extEvs", "Evictions caused by others"); partStat->append(&partInfo[p].profExtEvictions);
                partInfo[p].profInsertions.init("ins", "Number of insertion"); partStat->append(&partInfo[p].profInsertions);
                partInfo[p].profEvictions.init("evs", "Number of Evictions"); partStat->append(&partInfo[p].profEvictions);
                partInfo[p].profSizeCycles.init("szCycles", "Cumulative per-cycle sum of sz"); partStat->append(&partInfo[p].profSizeCycles);

                rpStat->append(partStat);
            }
            parentStat->append(rpStat);
        }

        void update(uint32_t id, const MemReq* req) {
            if (unlikely(zinfo->globPhaseCycles > lastUpdateCycle)) {
                //Update size-cycle counter stats
                uint64_t diff = zinfo->globPhaseCycles - lastUpdateCycle;
                for (uint32_t p = 0; p < partitions; p++) {
                    partInfo[p].profSizeCycles.inc(diff*partInfo[p].size);
                }

                profUpdateCycles.inc(diff);
                lastUpdateCycle = zinfo->globPhaseCycles;
            }

            LineInfo* e = &array[id];
            if (e->ts > 0) {
                assert(e->p < partitions);
                e->ts = timestamp++;
                partInfo[e->p].profHits.inc();
            } else { //post-miss update, old one has been removed, this is empty
                interMissCounter++;
                e->ts = timestamp++;
                partInfo[e->p].size--;
                partInfo[e->p].profEvictions.inc();
                partInfo[e->p].profSelfEvictions.inc(e->p == incomingLinePart);
                partInfo[e->p].profExtEvictions.inc(e->p != incomingLinePart);

                // for the new part
                assert(incomingLinePart < partitions);
                e->p = incomingLinePart;
                partInfo[e->p].size++;
                partInfo[e->p].interMisses++;
                partInfo[e->p].profInsertions.inc();
                partInfo[e->p].profMisses.inc();
            }

            //Profile the access
            monitor->access(e->p, e->addr);
        }

        void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } //sxj

        void startReplacement(const MemReq* req) {
            assert(candIdx == 0);
            assert(bestId == -1);
            incomingLinePart = mapper->getPartition(*req);
            incomingLineAddr = req->lineAddr;
        }

        void recordCandidate(uint32_t id) {
            assert(candIdx < assoc);
            candList[candIdx++] = id;
        }

        uint32_t getBestCandidate() {
            assert(candIdx > 0);
            assert(candIdx <= assoc);
            
            // the interval is end
            if (interMissCounter >= intervalLength) {
                int32_t maxEvictProb = -intervalLength;
                // update the scaling factor for all partitions
                for (uint32_t i = 0; i < partitions; i++) {
                    PrPartInfo & part = partInfo[i];
                    // calculate the eviction probability
                    int32_t wE = (int32_t)part.interBeginSize - (int32_t)part.targetSize + (int32_t)part.interMisses;
                    info("part: %d, size at beginning: %d, target size: %d, misses: %d, condition is: %d;", 
                        i, (int)part.interBeginSize, (int)part.targetSize, (int)part.interMisses, wE);
                    
                    if (wE <= 0) part.evictProb = 0;
                    else if (wE > (int32_t)intervalLength) part.evictProb = intervalLength;
                    else part.evictProb = wE;
                    
                    if (maxEvictProb < part.evictProb) {
                        maxEvictProb = part.evictProb;
                        bestPart = i;
                    // we break the tie by comparing their sizes
                    } else if (maxEvictProb == part.evictProb) {
                        if (part.size > partInfo[bestPart].size)
                            bestPart = i;
                    }
                    // reset the size/miss at the beginning of a interval
                    part.interBeginSize = part.size;
                    part.interMisses = 0;
                }

                interMissCounter = 0;
                assert(bestPart != partitions);
            }

            uint32_t bestCands[candIdx];
            uint32_t nonzeroCands[candIdx];
            uint32_t bestPos = 0;
            uint32_t nonzeroPos = 0;

            //record the candidates, we will choose the best one in next step
            for (uint32_t i = 0; i < candIdx; i++) {
                LineInfo* e = &array[candList[i]];
                // at the first interval, there are all empty lines, so the bestId cannot be -1
                if (e->ts == 0) {
                    //shortcut for empty positions
                    bestId = candList[i];
                    break;
                }

                // some eviction candidates belong to best partition
                if (e->p == bestPart) 
                    bestCands[bestPos++] = candList[i];
                // record the candidates with non-zero evcition probability
                else if (partInfo[e->p].evictProb > 0)
                    nonzeroCands[nonzeroPos++] = candList[i];
            }

            profNoCand.inc(!bestPos);
            
            // In the condition that no best candidates matching the bestPart, the non-zero candidates must exist.
            // If the numbers of best/non-zero cands are all 0, we shold have found a empty line. 
            if ( !( bestPos + nonzeroPos + (bestId >= 0) ) ) {
                // if all eviction probilities of partitions is zero, we evict the first candidate
                bestId = candList[0];
                //info("all Ei are zero, the eviction is: %d", candList[0]);
            }

            uint64_t timestamp = (uint64_t)-1;

            for (uint32_t i = 0; i < bestPos; i++)
                // choose the best cand by LRU
                if (array[ bestCands[i] ].ts < timestamp) {
                    timestamp = array[ bestCands[i] ].ts;
                    bestId = bestCands[i];
                }

            timestamp = (uint64_t)-1;
            // the best partition doesn't have any candidates
            for (uint32_t i = 0; (i < nonzeroPos) && !bestPos; i++)
                // choose the best cand by LRU
                if (array[ nonzeroCands[i] ].ts < timestamp) {
                    timestamp = array[ nonzeroCands[i] ].ts;
                    bestId = nonzeroCands[i];
                }

            assert(bestId >= 0 && (uint32_t)bestId < totalSize);
            return bestId;
        }

        void replaced(uint32_t id) {
            candIdx = 0; //reset
            bestId = -1;
            LineInfo* e = &array[id];
            e->ts = 0;
            e->addr = incomingLineAddr;
        }

    private:
        void setPartitionSizes(const uint32_t* sizes) {
            uint32_t s[partitions];

            for (uint32_t p = 0; p < partitions; p++) {
                assert(sizes[p] < partGranularity && sizes[p]); // the minimum size is 1, see the init.cpp

                s[p] = totalSize * sizes[p] / partGranularity;// according to the init.cpp, for vantage, the partGranularity is 256, thus the num of UMON bins is way num
#if UMON_INFO
                //info("part %d, %ld -> %d lines (now it's %ld lines); eviction prob: %d, granularity %d, alloc size %d, best partition: %d, no cands: %d;", 
                    //p, partInfo[p].targetSize, s[p], partInfo[p].size, partInfo[p].evictProb, partGranularity, sizes[p], bestPart, (int)profNoCand.get());
#endif
                // for FS partitioning, it doesn't need smoothtransient since it is replacement policy-based scheme
                partInfo[p].targetSize = s[p];
            }
        }
};

class HashFamily;
/* Protecting Distance based Partition scheme, the details are in MICRO 2012 , by shen */
class PDPartReplPolicy : public PartReplPolicy, public PDPReplPolicy {
    private:
        HashFamily* hf; // just for get the set index, but no common API can use

        struct PdPartInfo : public PartInfo {
            uint64_t pd;
            uint32_t accsCntr;
            Counter profInsertions;
            Counter profEvictions;
            Counter profSizeCycles;
        };
        
        PdPartInfo* partInfo;
        uint32_t partitions;
        //uint32_t partGranularity // there is no partition granularity at all !!
        //uint32_t totalSize; //i.e. numLines in PDPReplPolicy

        /*struct PartLineInfo { // has been defined in PDPReplPolicy
            Address addr; //FIXME: This is redundant due to the replacement policy interface
            uint64_t ts; //timestamp, >0 if in the cache, == 0 if line is empty (little significance otherwise)
            uint32_t p; //partition ID
            uint8_t rpd; // RPD in paper
            bool reuse; // the reuse bit
        };*/

        //PartLineInfo* array;

        Counter profUpdateCycles;

        //Repl process stuff
        //uint32_t* candList; replacing by the candArray in PDPReplPolicy
        uint32_t incomingLinePart; //to what partition does the incoming line belong?
        Address incomingLineAddr;

        //Globally incremented, but bears little significance per se
        uint64_t timestamp; // global timestamp

        uint64_t lastUpdateCycle; //for cumulative size counter updates; could be made event-driven

    public:
        // the monitor for Futility Scaling scheme is UCP for default
        PDPartReplPolicy(PartitionMonitor* _monitor, PartMapper* _mapper, HashFamily* _hf, uint32_t _lines, 
            uint32_t _assoc, uint32_t _maxSd, bool _nonInclusive, uint32_t _period)
        : PartReplPolicy(_monitor, _mapper), PDPReplPolicy(_lines, _assoc, nullptr, _maxSd, _nonInclusive, _period), hf(_hf)
        {
            partitions = mapper->getNumPartitions();
            partInfo = gm_calloc<PdPartInfo>(partitions);

            for (uint32_t i = 0; i < partitions; i++) {
                partInfo[i].pd = (uint8_t) -1; // it's casual, just 8
                //Need placement new, these objects have vptr
                new (&partInfo[i].profHits) Counter;
                new (&partInfo[i].profMisses) Counter;
                new (&partInfo[i].profSelfEvictions) Counter;
                new (&partInfo[i].profExtEvictions) Counter;
                new (&partInfo[i].profInsertions) Counter;
                new (&partInfo[i].profEvictions) Counter;
                new (&partInfo[i].profSizeCycles) Counter;
            }

            //array = gm_calloc<PartLineInfo>(numLines);
            assert(numLines >= partitions);

            for (uint32_t i = 0; i < numLines; i++) {
                array[i].p = 0; // just for now, we set all lines to the first partition
            }
            partInfo[0].size = numLines; // change the actual size

            timestamp = 1;
            lastUpdateCycle = 0;
            info("Protecting Distance based RP: %d partitions", partitions);
        }

        ~PDPartReplPolicy() { gm_free(partInfo); }

        void initStats(AggregateStat* parentStat) {
            AggregateStat* rpStat = new AggregateStat();
            rpStat->init("part", "Futility Scaling replacement policy stats");
            ProxyStat* pStat;
            //profUpdateCycles.init("updCycles", "Cycles of updates experienced on size-cycle counters"); rpStat->append(&profUpdateCycles);
            for (uint32_t p = 0; p < partitions; p++) {
                std::stringstream pss;
                pss << "part-" << p;
                AggregateStat* partStat = new AggregateStat();
                partStat->init(gm_strdup(pss.str().c_str()), "Partition stats");

                pStat = new ProxyStat(); pStat->init("sz", "Actual size", &partInfo[p].size); partStat->append(pStat);
                //FIXME: Code and stats should be named similarly
                pStat = new ProxyStat(); pStat->init("tgtSz", "Target size", &partInfo[p].targetSize); partStat->append(pStat);
                partInfo[p].profHits.init("hits", "Hits"); partStat->append(&partInfo[p].profHits);
                partInfo[p].profMisses.init("misses", "Misses"); partStat->append(&partInfo[p].profMisses);
                partInfo[p].profSelfEvictions.init("selfEvs", "Evictions caused by us"); partStat->append(&partInfo[p].profSelfEvictions);
                partInfo[p].profExtEvictions.init("extEvs", "Evictions caused by others"); partStat->append(&partInfo[p].profExtEvictions);
                partInfo[p].profInsertions.init("ins", "Number of insertion"); partStat->append(&partInfo[p].profInsertions);
                partInfo[p].profEvictions.init("evs", "Number of Evictions"); partStat->append(&partInfo[p].profEvictions);
                pStat = new ProxyStat(); pStat->init("PD", "Protecting Distance", &partInfo[p].pd); partStat->append(pStat);
                partInfo[p].profSizeCycles.init("szCycles", "Cumulative per-cycle sum of sz"); partStat->append(&partInfo[p].profSizeCycles);

                rpStat->append(partStat);
            }
            parentStat->append(rpStat);
        }

        void update(uint32_t id, const MemReq* req) {
            ++totAccs;
            if (unlikely(zinfo->globPhaseCycles > lastUpdateCycle)) {
                //Update size-cycle counter stats
                uint64_t diff = zinfo->globPhaseCycles - lastUpdateCycle;
                for (uint32_t p = 0; p < partitions; p++) {
                    partInfo[p].profSizeCycles.inc(diff*partInfo[p].size);
                }

                profUpdateCycles.inc(diff);
                lastUpdateCycle = zinfo->globPhaseCycles;
            }

            LineInfo* e = &array[id];
            partInfo[e->p].accsCntr++;
            assert(e->p < partitions);

            if (e->ts > 0) {
                assert(e->addr == req->lineAddr);
                e->ts = timestamp++;
                partInfo[e->p].profHits.inc();
            } else { //post-miss update, old one has been removed, this is empty
                e->ts = timestamp++;
                partInfo[e->p].size--;
                partInfo[e->p].profEvictions.inc();
                partInfo[e->p].profSelfEvictions.inc(e->p == incomingLinePart);
                partInfo[e->p].profExtEvictions.inc(e->p != incomingLinePart);
                //zinfo("FS update: partition %d, number of evictions %d", (int)e->p, (int)partInfo[e->p].profEvictions.get());
                partInfo[e->p].profMisses.inc();

                // for the new part
                assert(incomingLinePart < partitions);
                e->p = incomingLinePart;
                partInfo[e->p].size++;
                partInfo[e->p].profInsertions.inc();
            }

            //Profile the access
            monitor->access(e->p, e->addr);
            uint32_t set = hf->hash(0, e->addr) & (numSets - 1);
            ++distanceStep[set]; // every access, the distance step counter increase by 1
            uint32_t first = set * assoc;
            // if the distance step is maxSd, we decrement all RPDs of lines in this set
            if (distanceStep[set] > maxSd - 1) {
                distanceStep[set] = 0;
                for (uint32_t id = first; id < assoc + first; ++id)
                    array[id].rpd -= array[id].rpd != 0; // decreae every access to this set, the lines' saturating counter decrease
            }
        }

        void hitUpdate(uint32_t id, const MemReq* req) { 
            array[id].reuse = true; 
            array[id].rpd = partInfo[array[id].p].pd / maxSd;

            update(id, req);
        }

        //void hitUpdate(uint32_t id, const MemReq* req) { update(id, req); } //no need to override this method

        void startReplacement(const MemReq* req) {
            assert(candIdx == 0);
            assert(bestId == -1);
            incomingLinePart = mapper->getPartition(*req);
            incomingLineAddr = req->lineAddr;
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

            if (totAccs < period)
                return bestId;

            // calculate the best PD every 512K access
            totAccs = 0;
            uint64_t numerator = 0; // Eq.2
            uint64_t demoninator = 0; // Eq.2
            uint8_t pdNum = 3;

            for (uint32_t p = 0; p < partitions; ++p) {
                uint8_t peaks[pdNum]; // the first three best PDs
                uint32_t hits[pdNum]; // the corresponding hits (numerator in Eq.1)
                uint64_t accsW[pdNum]; // the corresponding Access*W (demoninator in Eq.1)
                double emMax = 0.0; // the maximum of overall hit rate for this partition
                int8_t bestPdIdx = -1;
                calcBestPd(partInfo[p].accsCntr, &peaks[0], &hits[0], &accsW[0], pdNum, reinterpret_cast<ReuseDistSampler*>(monitor->getMonitor(p)) );
                // find the best PD for each partition
                for (uint32_t i = 0; i < pdNum; ++i) {
                    uint64_t tempNume = numerator + hits[i];
                    uint64_t tempDemo = demoninator + accsW[i];
                    double em = (double)tempNume / tempDemo;
                    if (em > emMax) {
                        bestPdIdx = i; // record the best PD's index for partition p
                        emMax = em;
                    }
                }
                
                partInfo[p].pd = peaks[bestPdIdx]; // update the PD for this part
                numerator += hits[bestPdIdx];
                demoninator += accsW[bestPdIdx];
                
                info("partition %d: best PD = %d, hit rate = %f", p, peaks[bestPdIdx], (double)numerator / demoninator);
            }

            return bestId;
        }

        void replaced(uint32_t id) {
            LineInfo* e = &array[id];
            e->rpd = partInfo[e->p].pd / maxSd;
            e->reuse = false;
            candIdx = 0; //reset
            bestId = -1;
            e->ts = 0;
            e->addr = incomingLineAddr;
        }

    private:
        void setPartitionSizes(const uint32_t* sizes) {}
};

#endif  // PART_REPL_POLICIES_H_