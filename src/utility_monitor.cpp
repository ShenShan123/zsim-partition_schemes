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

// ==================== Histogram class ====================, by shen

template <class B>
Histogram<B>::Histogram(int _s) : samples(0), size(_s)
{
    bins = gm_calloc<B>(size);
    /* init bins to 0 */
    for (int i = 0; i < size; ++i)
        bins[i] = 0;
}

template <class B>
Histogram<B>::Histogram(const Histogram<B> & rhs) : samples(rhs.samples), size(rhs.size)
{
    bins = new B[size];

    for (int i = 0; i < size; ++i)
        bins[i] = rhs.bins[i];
}

template <class B>
Histogram<B>::~Histogram() { gm_free(bins); }

template <class B>
void Histogram<B>::setSize(int _s)
{
    if (bins != nullptr)
        gm_free(bins);

    size = _s;
    bins = gm_calloc<B>(size);

    /* init bins to 0 */
    for (int i = 0; i < size; ++i)
        bins[i] = 0;

    //std::cout << "size of bins " << size << std::endl;
}

template <class B>
const int Histogram<B>::getSize() { return size; }

template <class B>
void Histogram<B>::clear()
{
    samples = 0;
    /* when clear bins, the size keeps unchanged */
    for (int i = 0; i < size; ++i)
        bins[i] = 0;
}

template <class B>
void Histogram<B>::normalize()
{
    for (int i = 0; i < size; ++i)
        bins[i] /= samples;
}

template <class B>
B & Histogram<B>::operator[](const int idx) const
{
    assert(idx >= 0 && idx < size);
    return bins[idx];
}

template <class B>
Histogram<B> & Histogram<B>::operator=(const Histogram<B> & rhs)
{
    assert(size == rhs.size());

    for (int i = 0; i < size; ++i)
        bins[i] = rhs.bins[i];

    samples = rhs.samples;
    return *this;
}

template <class B>
Histogram<B> & Histogram<B>::operator+=(const Histogram<B> & rhs)
{
    assert(size == rhs.size);

    for (int i = 0; i < size; ++i)
        bins[i] += rhs.bins[i];

    samples += rhs.samples;
    return *this;
}

/*
template <class B>
void Histogram<B>::sample(int x, uint16_t n)
{
    // the sample number must less than max size of bins
    assert(x < size && x >= 0);
    bins[x] += n;
    // calculate the total num of sampling
    samples += n;
}
*/
template <class B>
void Histogram<B>::print()
//void Histogram<B>::print(std::ofstream & file)
{
    //file.write((char *)bins, sizeof(B) * size);
    //for (int i = 0; i < size; ++i)
        //file << bins[i] << " ";
    //file  << "\n";

    // just for test
    info("RDV print: [0] %d, [%d] %d, total samples: %d", bins[0], size - 1, bins[size-1], samples);
    std::ofstream outfile("rdvout.dat", ios::app); // **sxj used for set up an output file
    //outfile << "hit counts = " << hitCounts <<", miss counts = " << missCounts << endl; // **sxj
    //uint32_t hitRate = hitCounts/(hitCounts+missCounts);
    //outfile << "hit counts = " << hitCounts <<", miss counts = " << missCounts << ", hit rate = " << hitRate << endl; // **sxj
    for (int i = 0; i < size; i++) {
        outfile << " " << bins[i]; // **sxj 
    }
    outfile << endl;
    outfile.close();

}

/*
template <class B>
B Histogram<B>::getValue(int i) const {
    assert(x < size && x >= 0);
    return bins[i];
}
*/

// ==================== ReuseDistSampler class ====================, implemented by shen
ReuseDistSampler::ReuseDistSampler(HashFamily* _hf, uint32_t _bankSets, uint32_t _samplerSets, uint32_t _buckets, uint32_t _max, uint32_t _window) 
    : hitCounts(0), missCounts(0), // **sxj
    indices(nullptr), intervalLength(_max), sampleWindow(_window), step((_max + 1) / _buckets), sampleCntrs(nullptr), residuals(nullptr), maxRd(_max),
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

    //hf = new H3HashFamily(2, 32, 0xF000BAAD);
    rdv = new Histogram<uint32_t>(_buckets);
    rdvSize = rdv->getSize();

    info("ReuseDistSampler, sampling rate 1/%d, number of sampler sets %d, max RD %d, RDV buckets: %d, sampling window size %d", dssRate, samplerSets, _max, rdv->getSize(), _window);
}

ReuseDistSampler::~ReuseDistSampler() 
{ 
    gm_free(indices);
    
    if (sampleCntrs != nullptr && residuals != nullptr) {
        gm_free(sampleCntrs);
        gm_free(residuals);
    }
    delete rdv;
};

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

const uint32_t ReuseDistSampler::getSet(uint64_t addr) { return hf->hash(0, addr) & (bankSets - 1); }

void ReuseDistSampler::access(uint64_t addr)
{
    uint32_t set = getSet(addr);
    // do we sample this line? if not, just return
    if (set & (dssRate - 1))
        return;

    uint32_t ss = set / dssRate; // ss is the sampler set index
    assert(ss < samplerSets);

    uint32_t & index = ++indices[ss];
    // increase the index counter and see if a new interval starts!
    /*if ( (++index & (intervalLength - 1)) == 0 ) {
        index = 0;
        //uint32_t delNodes = cleanOldEntry();
        rdv.print();
        rdv.clear();
        //info("addrMap old size: %d, new size: %d", (int)addrMap.size() + delNodes, (int)addrMap.size());
        info("addrMap new size: %d", (int)addrMap.size());
    }*/

    auto pos = addrMap.find(addr);

    if (pos != addrMap.end()) {
        hitCounts++; // **sxj
        uint32_t rd = index - pos->second - 1; 
        //info("rd = %d", rd);
        // for the sampling scheme, rd-counter is clear when finish rd calculation
        if (sampleWindow)
            addrMap.erase(pos);
        // if the rd larger than the truncation, we limite the maximum RD to maxRd
        if (rd >= maxRd) {
            //cleanOldEntry(rdv);
            rdv->sample(DOLOG(maxRd) / step); // the first step rds account for the increment of rdv[0], the second ones for the increment of rdv[1], and so on
        }
        // else we don't need to traverse the addrMap 
        else
            rdv->sample(DOLOG(rd) / step);
    }
    else
        missCounts++; // **sxj

    // for sampline scheme 
    if (sampleWindow && sampleCntrs[ss]) {
        --sampleCntrs[ss];
    }
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
    else {
        addrMap[addr] = index;
    }
}

void ReuseDistSampler::print() { // **sxj
    std::ofstream outfile("rdvout.dat", ios::app); // **sxj 用于建立输出文件
    double hitRate = 100*((double)hitCounts)/((double)(hitCounts+missCounts)); // **sxj change to the double for percentage
    outfile << "hit counts = " << hitCounts <<", miss counts = " << missCounts << ", hit rate = " << hitRate << endl; // **sxj
    outfile.close(); // **sxj
}

void ReuseDistSampler::clear() {
    info("cleaning, addrMap size: %d", (int)addrMap.size());
    addrMap.clear(); 
    print(); // **sxj try to call another func
    rdv->print();
    rdv->clear();
    hitCounts = 0; // **sxj
    missCounts = 0; // **sxj


    for (uint32_t i = 0; i < samplerSets; ++i) {
        indices[i] = 0;
        if (sampleWindow) {
            sampleCntrs[i] = 0;
            residuals[i] = 0;
        }
    }
}
