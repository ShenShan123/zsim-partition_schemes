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

#ifndef PARTITIONER_H_
#define PARTITIONER_H_

#include "event_queue.h"
#include "g_std/g_vector.h"
#include "galloc.h"
#include "memory_hierarchy.h"
#include "stats.h"
#include "utility_monitor.h"

class PartReplPolicy;

// allocates space in a cache between multiple partitions
class Partitioner : public GlobAlloc {
    public:
        Partitioner(uint32_t _minAlloc, double _allocPortion, bool* _forbidden)
                : minAlloc(_minAlloc), allocPortion(_allocPortion), forbidden(_forbidden) {}

        class PartitionEvent: public Event {
            private:
                Partitioner* part;
            public:
                PartitionEvent(Partitioner* _part, uint64_t _period) : Event(_period), part(_part) {}
                void callback() { part->partition(); }
        };
        virtual void partition() = 0;

    protected:
        uint32_t minAlloc;
        double allocPortion;
        bool* forbidden;
};

// Gives best partition sizes as estimated with the greedy lookahead
// algorithm proposed in the UCP paper (Qureshi and Patt, ISCA 2006)
namespace lookahead {
    uint64_t computePartitioningTotalUtility(uint32_t numPartitions, const uint32_t* parts, const uint32_t* missCurves);
    void computeBestPartitioning(uint32_t numPartitions, uint32_t* allocs, uint32_t* missCurves);
}

class LookaheadPartitioner : public Partitioner {
    public:
        LookaheadPartitioner(PartReplPolicy* _repl, uint32_t _numPartitions, uint32_t _buckets, 
            uint32_t _minAlloc = 1, double _allocPortion = 1.0, bool* _forbidden = nullptr);

        void partition();
    
    private:
        PartReplPolicy* repl;
        uint32_t numPartitions;
        uint32_t buckets;
        uint32_t* curAllocs;
};

class PDPartitioner : public Partitioner { // TODO: the PD calculation could be implemented here, by shen
    public:
        PDPartitioner(PartReplPolicy* _repl, uint32_t _numPartitions, uint32_t _minAlloc = 1, double _allocPortion = 1.0, bool* _forbidden = nullptr)
        : Partitioner(_minAlloc, _allocPortion, _forbidden)
        , repl(_repl)
        , numPartitions(_numPartitions) {}

        void partition() {};

    private:
        PartReplPolicy* repl;
        uint32_t numPartitions;
}; // end, by shen

// *********************************************************************

// monitors the usage of partitions in a cache and generates miss curves
class PartitionMonitor : public GlobAlloc {
    public:
        explicit PartitionMonitor(uint32_t _buckets) : buckets(_buckets) {}

        virtual uint32_t getNumPartitions() const = 0;

        // called by PartReplPolicy on a memory reference
        virtual void access(uint32_t partition, Address lineAddr) = 0;

        // called by Partitioner to get misses
        virtual uint32_t get(uint32_t partition, uint32_t bucket) const = 0;

        virtual uint32_t getNumAccesses(uint32_t partition) const = 0;

        // called by Partitioner each interval to reset miss counters
        virtual void reset() = 0;

        uint32_t getBuckets() const { return buckets; }

        virtual void* getMonitor(uint32_t p) const = 0; // add by shen

    protected:
        uint32_t buckets;
};

// Maintains UMONs for each partition as in (Qureshi and Patt, ISCA 2006).
// Stupid name...but what do you call it? -nzb
class UMonMonitor : public PartitionMonitor {
    public:
        UMonMonitor(uint32_t _numLines, uint32_t _umonLines, uint32_t _umonBuckets, uint32_t _numPartitions, uint32_t _buckets);
        ~UMonMonitor();

        uint32_t getNumPartitions() const { return monitors.size(); }
        void access(uint32_t partition, Address lineAddr);
        uint32_t get(uint32_t partition, uint32_t bucket) const;
        uint32_t getNumAccesses(uint32_t partition) const;
        void reset();
        void* getMonitor(uint32_t p) const; // add by shen

    private:
        void getMissCurves() const;
        void getMissCurve(uint32_t* misses, uint32_t partition) const;

        mutable uint32_t* missCache;
        mutable bool missCacheValid;
        g_vector<UMon*> monitors;       // individual monitors per partition
};

// add the reuse distance monitor for partitioning scheme, by shen
class ReuseDistMonitor : public PartitionMonitor{
    public:
        ReuseDistMonitor(uint32_t _numPartitions, HashFamily* _hf, uint32_t _bankSets, uint32_t _samplerSets, uint32_t _buckets, uint32_t _max, uint32_t _window);
        ~ReuseDistMonitor();

        uint32_t getNumPartitions() const { return monitors.size(); }
        void access(uint32_t partition, Address lineAddr);
        // these two methods are useless in RD monitor
        uint32_t get(uint32_t partition, uint32_t bucket) const { return 0; };
        uint32_t getNumAccesses(uint32_t partition) const { return 0; };
        void reset();
        void* getMonitor(uint32_t p) const;

    private:
        g_vector<ReuseDistSampler*> monitors;
};

#endif  // PARTITIONER_H_
