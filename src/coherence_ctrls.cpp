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

#include "coherence_ctrls.h"
#include "cache.h"
#include "network.h"

/* Do a simple XOR block hash on address to determine its bank. Hacky for now,
 * should probably have a class that deals with this with a real hash function
 * (TODO)
 */
uint32_t MESIBottomCC::getParentId(Address lineAddr) {
    //Hash things a bit
    uint32_t res = 0;
    uint64_t tmp = lineAddr;
    for (uint32_t i = 0; i < 4; i++) {
        res ^= (uint32_t) ( ((uint64_t)0xffff) & tmp);
        tmp = tmp >> 16;
    }
    return (res % parents.size());
}

void MESIBottomCC::init(const g_vector<MemObject*>& _parents, Network* network, const char* name) {
    parents.resize(_parents.size());
    parentRTTs.resize(_parents.size());
    for (uint32_t p = 0; p < parents.size(); p++) {
        parents[p] = _parents[p];
        parentRTTs[p] = (network)? network->getRTT(name, parents[p]->getName()) : 0;
    }
}

uint64_t MESIBottomCC::processEviction(Address wbLineAddr, uint32_t lineId, bool lowerLevelWriteback, uint64_t cycle, uint32_t srcId) {
    MESIState* state = &array[lineId];
    if (lowerLevelWriteback) {
        //If this happens, when tcc issued the invalidations, it got a writeback. This means we have to do a PUTX, i.e. we have to transition to M if we are in E
        assert(*state == M || *state == E); //Must have exclusive permission!
        *state = M; //Silent E->M transition (at eviction); now we'll do a PUTX
    }
    uint64_t respCycle = cycle;
    switch (*state) {
        case I:
            break; //Nothing to do
        case S:
        case E:
            {
                MemReq req = {wbLineAddr, PUTS, selfId, state, cycle, &ccLock, *state, srcId, 0 /*no flags*/};
                respCycle = parents[getParentId(wbLineAddr)]->access(req);
            }
            break;
        case M:
            {
                MemReq req = {wbLineAddr, PUTX, selfId, state, cycle, &ccLock, *state, srcId, 0 /*no flags*/};
                respCycle = parents[getParentId(wbLineAddr)]->access(req);
            }
            break;

        default: panic("!?");
    }
    assert_msg(*state == I, "Wrong final state %s on eviction, lineAddr %ld, lineId %d selfId %d srcId %d", MESIStateName(*state), wbLineAddr, lineId, selfId, srcId);
    return respCycle;
}

uint64_t MESIBottomCC::processAccess(Address lineAddr, uint32_t lineId, AccessType type, uint64_t cycle, uint32_t srcId, uint32_t flags) {
    uint64_t respCycle = cycle;

    MESIState* state = &array[lineId];
    //info("bcc access: lineAddr %ld, type %s, lineId %d, srcId %d, sate %s", lineAddr, AccessTypeName(type), lineId, srcId, MESIStateName(*state));
    switch (type) {
        // A PUTS/PUTX does nothing w.r.t. higher coherence levels --- it dies here
        case PUTS: //Clean writeback, nothing to do (except profiling)
            assert(*state != I);
            profPUTS.inc();
            break;
        case PUTX: //Dirty writeback
            assert(*state == M || *state == E);
            if (*state == E) {
                //Silent transition, record that block was written to
                *state = M;
            }
            profPUTX.inc();
            break;
        case GETS:
            if (*state == I) {
                uint32_t parentId = getParentId(lineAddr);
                MemReq req = {lineAddr, GETS, selfId, state, cycle, &ccLock, *state, srcId, flags};
                uint32_t nextLevelLat = parents[parentId]->access(req) - cycle;
                uint32_t netLat = parentRTTs[parentId];
                profGETNextLevelLat.inc(nextLevelLat);
                profGETNetLat.inc(netLat);
                respCycle += nextLevelLat + netLat;
                profGETSMiss.inc();
                assert(*state == S || *state == E);
            } else {
                profGETSHit.inc();
            }
            break;
        case GETX:
            if (*state == I || *state == S) {
                //Profile before access, state changes
                if (*state == I) profGETXMissIM.inc();
                else profGETXMissSM.inc();
                uint32_t parentId = getParentId(lineAddr);
                MemReq req = {lineAddr, GETX, selfId, state, cycle, &ccLock, *state, srcId, flags};
                // if this cacheline is invalidated or shared, read from the next level memory directly, by shen
                uint32_t nextLevelLat = parents[parentId]->access(req) - cycle;
                uint32_t netLat = parentRTTs[parentId];
                profGETNextLevelLat.inc(nextLevelLat);
                profGETNetLat.inc(netLat);
                respCycle += nextLevelLat + netLat;
            } else {
                if (*state == E) {
                    // Silent transition
                    // NOTE: When do we silent-transition E->M on an ML hierarchy... on a GETX, or on a PUTX?
                    /* Actually, on both: on a GETX b/c line's going to be modified anyway, and must do it if it is the L1 (it's OK not
                     * to transition if L2+, we'll TX on the PUTX or invalidate, but doing it this way minimizes the differences between
                     * L1 and L2+ controllers); and on a PUTX, because receiving a PUTX while we're in E indicates the child did a silent
                     * transition and now that it is evictiong, it's our turn to maintain M info.
                     */
                    *state = M;
                }
                profGETXHit.inc();
            }
            assert_msg(*state == M, "Wrong final state on GETX, lineId %d numLines %d, finalState %s", lineId, numLines, MESIStateName(*state));
            break;

        default: panic("!?");
    }
    assert_msg(respCycle >= cycle, "XXX %ld %ld", respCycle, cycle);
    return respCycle;
}

uint64_t MESIBottomCC::processBypass(uint64_t lineAddr, AccessType type, uint32_t childId, MESIState* state, uint64_t cycle, MESIState initialState, uint32_t srcId, uint32_t flags) { // add by shen
    uint64_t respCycle = cycle;
    uint32_t parentId = getParentId(lineAddr);
    info("process bypassing, access the parent cache");
    MemReq thisReq = {lineAddr, type, childId, state, cycle, &ccLock, initialState, srcId, flags}; // just change the lock

    uint32_t nextLevelLat = parents[parentId]->access(thisReq) - cycle;
    uint32_t netLat = parentRTTs[parentId];
    profGETNextLevelLat.inc(nextLevelLat);
    profGETNetLat.inc(netLat);
    respCycle += nextLevelLat + netLat;
    //profGETSMiss.inc();
    assert_msg(respCycle >= cycle, "XXX %ld %ld", respCycle, cycle);
    return respCycle;
}

void MESIBottomCC::processWritebackOnAccess(Address lineAddr, uint32_t lineId, AccessType type) {
    MESIState* state = &array[lineId];
    assert(*state == M || *state == E);
    if (*state == E) {
        //Silent transition to M if in E
        *state = M;
    }
}

void MESIBottomCC::processInval(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback) {
    MESIState* state = &array[lineId];
    assert(*state != I);
    switch (type) {
        case INVX: //lose exclusivity
            //Hmmm, do we have to propagate loss of exclusivity down the tree? (nah, topcc will do this automatically -- it knows the final state, always!)
            assert_msg(*state == E || *state == M, "Invalid state %s", MESIStateName(*state));
            if (*state == M) *reqWriteback = true;
            *state = S;
            profINVX.inc();
            break;
        case INV: //invalidate
            assert(*state != I);
            if (*state == M) *reqWriteback = true;
            *state = I;
            profINV.inc();
            break;
        case FWD: //forward
            assert_msg(*state == S, "Invalid state %s on FWD", MESIStateName(*state));
            profFWD.inc();
            break;
        default: panic("!?");
    }
    //NOTE: BottomCC never calls up on an invalidate, so it adds no extra latency
}


uint64_t MESIBottomCC::processNonInclusiveWriteback(Address lineAddr, AccessType type, uint64_t cycle, MESIState* state, uint32_t srcId, uint32_t flags) {
    if (!nonInclusiveHack) panic("Non-inclusive %s on line 0x%lx, this cache should be inclusive", AccessTypeName(type), lineAddr);

    MemReq req = {lineAddr, type, selfId, state, cycle, &ccLock, *state, srcId, flags | MemReq::NONINCLWB};
    uint64_t respCycle = parents[getParentId(lineAddr)]->access(req);
    return respCycle;
}

/* MESITopCC implementation */

void MESITopCC::init(const g_vector<BaseCache*>& _children, Network* network, const char* name) {
    if (_children.size() > MAX_CACHE_CHILDREN) {
        panic("[%s] Children size (%d) > MAX_CACHE_CHILDREN (%d)", name, (uint32_t)_children.size(), MAX_CACHE_CHILDREN);
    }
    children.resize(_children.size());
    childrenRTTs.resize(_children.size());
    for (uint32_t c = 0; c < children.size(); c++) {
        children[c] = _children[c];
        childrenRTTs[c] = (network)? network->getRTT(name, children[c]->getName()) : 0;
    }
}

uint64_t MESITopCC::sendInvalidates(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId) {
    //Send down downgrades/invalidates
    Entry* e = &array[lineId];

    //Don't propagate downgrades if sharers are not exclusive.
    if (type == INVX && !e->isExclusive()) {
        return cycle;
    }

    uint64_t maxCycle = cycle; //keep maximum cycle only, we assume all invals are sent in parallel
    if (!e->isEmpty()) {
        uint32_t numChildren = children.size();
        uint32_t sentInvs = 0;
        for (uint32_t c = 0; c < numChildren; c++) {
            if (e->sharers[c]) {
                InvReq req = {lineAddr, type, reqWriteback, cycle, srcId};
                uint64_t respCycle = children[c]->invalidate(req);
                respCycle += childrenRTTs[c];
                maxCycle = MAX(respCycle, maxCycle);
                if (type == INV) e->sharers[c] = false;
                sentInvs++;
            }
        }
        assert(sentInvs == e->numSharers);
        if (type == INV) {
            e->numSharers = 0;
        } else { // type == FWD, by shen
            //TODO: This is kludgy -- once the sharers format is more sophisticated, handle downgrades with a different codepath
            assert(e->exclusive);
            assert(e->numSharers == 1);
            e->exclusive = false;
        }
        assert(e->numSharers == e->sharers.count());
    }
    return maxCycle;
}


uint64_t MESITopCC::processEviction(Address wbLineAddr, uint32_t lineId, bool* reqWriteback, uint64_t cycle, uint32_t srcId) {
    if (nonInclusiveHack) {
        // Don't invalidate anything, just clear our entry
        array[lineId].clear();
        return cycle;
    } else { // if it is a inclusive cache, the upper level cacheline is evicted, the corresponding line in lower level cache must be invalidated!!! by shen.
        //Send down invalidates
        return sendInvalidates(wbLineAddr, lineId, INV, reqWriteback, cycle, srcId);
    }
}

uint64_t MESITopCC::processAccess(Address lineAddr, uint32_t lineId, AccessType type, uint32_t childId, bool haveExclusive,
                                  MESIState* childState, bool* inducedWriteback, uint64_t cycle, uint32_t srcId, uint32_t flags) {
    Entry* e = &array[lineId];
    uint64_t respCycle = cycle;
    /*info("tcc access: lineAddr %ld, type %s, lineId %d childId %d sharers %d, srcId %d,  childState %s, isExclusive %d numSharers %d",
                lineAddr, AccessTypeName(type), lineId, childId, (int)e->sharers[childId], srcId, MESIStateName(*childState), e->exclusive, e->numSharers);*/
    
    // we modified this part of this routine for configring a nonInclusive L2 and nonInclusive L3 cache. Note the condition that the L2 do not have the copy of line in L1,
    // as well as L3. So the L3 cache has no sharers of L2, thus we need to handle for the PUTX/PUTS otherwise the assertion will fail. by shen
    switch (type) {
        case PUTX: // handle the dirty write back to upper level cache and change the child cache's state (lower level), by shen
            assert_msg(/*e->isExclusive()*/ e->exclusive, "Wrong state for PUTX, address %lx, lineId %d childId %d sharers %d, srcId %d,  childState %s, isExclusive %d numSharers %d", 
                lineAddr, lineId, childId, (int)e->sharers[childId], srcId, MESIStateName(*childState), e->exclusive, e->numSharers);
            if ((flags & MemReq::PUTX_KEEPEXCL) && e->numSharers) { // if this and the child cache do not have the copy, the e->isExclusive() could fail an assert.
                assert(e->sharers[childId]);
                assert(*childState == M);
                *childState = E; //they don't hold dirty data anymore
                break; //don't remove from sharer set. It'll keep exclusive perms.
            }
            else if ((flags & MemReq::PUTX_KEEPEXCL) && (e->numSharers == 0)) { // if the child level cache also has no copy, just do change the state of cacheline in L1 cache, by shen
                assert(*childState == M);
                *childState = E; //they don't hold dirty data anymore
                break;                
            }
            else if (e->numSharers == 0) { // if the child level cache has no copy, just invalide the L1 cache
                *childState = I;
            }
            //note NO break in general
        case PUTS:// a clean write back from child cache indicate the line in lower cache has been evicted in exclusive cache. by shen
            if (e->numSharers && !e->sharers[childId]) { // if this line in this level does have any sharers, but not the current childId, just invalide the childState. by shen
                /*assert_msg(e->sharers[childId], "Wrong state for PUTS, address %ld, lineId %d childId %d sharers %d, srcId %d,  childState %s, isExclusive %d numSharers %d %d", 
                    lineAddr, lineId, childId, (int)e->sharers[childId], srcId, MESIStateName(*childState), e->exclusive, e->numSharers, (int)e->sharers.count());*/ // commented by shen
                //e->sharers[childId] = false;
            }
            else if (e->numSharers && e->sharers[childId]) { // if this line has sharer and the sharer is this child cache, take it from the sharer list, by shen
                e->sharers[childId] = false;
                e->numSharers--;
            }
            // if no sharers, it indicates the L2 is also nonInclusive, so just change state of the cacheline in L1, by shen
            *childState = I;
            break;
        case GETS: // fetch a clean line by the child cache
            if (e->isEmpty() && haveExclusive && !(flags & MemReq::NOEXCL)) {
                //Give in E state
                e->exclusive = true;
                e->sharers[childId] = true;
                e->numSharers = 1;
                *childState = E;
            } else {
                //Give in S state
                assert(e->sharers[childId] == false);
                // if the line is exclusive cacheline, invalidate the sharers.
                if (e->isExclusive()) {
                    //Downgrade the exclusive sharer
                    respCycle = sendInvalidates(lineAddr, lineId, INVX, inducedWriteback, cycle, srcId);
                }

                assert_msg(!e->isExclusive(), "Can't have exclusivity here. isExcl=%d excl=%d numSharers=%d", e->isExclusive(), e->exclusive, e->numSharers);

                e->sharers[childId] = true;
                e->numSharers++;
                e->exclusive = false; //dsm: Must set, we're explicitly non-exclusive
                *childState = S;
            }
            break;
        case GETX: // fetch a exclusive cacheline by the child cache, so we need to invalidate the other sharers. by shen
            assert(haveExclusive); //the current cache better have exclusive access to this line

            // If child is in sharers list (this is an upgrade miss), take it out now and add it after invoking sendInvalidates(), by shen
            if (e->sharers[childId]) {
                assert_msg(!e->isExclusive(), "Spurious GETX, childId=%d numSharers=%d isExcl=%d excl=%d", childId, e->numSharers, e->isExclusive(), e->exclusive);
                e->sharers[childId] = false;
                e->numSharers--;
            }

            // Invalidate all other copies
            respCycle = sendInvalidates(lineAddr, lineId, INV, inducedWriteback, cycle, srcId);

            // Set current sharer, mark exclusive
            e->sharers[childId] = true;
            e->numSharers++;
            e->exclusive = true;

            assert(e->numSharers == 1);

            *childState = M; //give in M directly
            break;

        default: panic("!?");
    }

    /*if ((e->numSharers && !e->sharers[childId]) || (e->numSharers != e->sharers.count())) {
        for (uint32_t i = 0; i < e->sharers.size(); ++i)
            info("sharer [%d] is shared? %d", i, (int)e->sharers[i]);
        info("Something wrong in sharers, address %ld, lineId %d childId %d sharers %d, srcId %d,  childState %s, isExclusive %d numSharers %d %d, type %s", 
                    lineAddr, lineId, childId, (int)e->sharers[childId], srcId, MESIStateName(*childState), e->exclusive, e->numSharers, (int)e->sharers.count(), AccessTypeName(type));
        exit(1);
    }*/


    return respCycle;
}

uint64_t MESITopCC::processInval(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId) {
    if (type == FWD) {//if it's a FWD, we should be inclusive for now, so we must have the line, just invLat works
        assert(!nonInclusiveHack); //dsm: ask me if you see this failing and don't know why
        return cycle;
    } else {
        //Just invalidate or downgrade down to children as needed
        return sendInvalidates(lineAddr, lineId, type, reqWriteback, cycle, srcId);
    }
}

