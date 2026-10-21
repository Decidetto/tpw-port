// CMessageCentre::Save 0x10152758, Load 0x10152840, UnsubscribeAll 0x10152448 (unit v5760).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <new>
#include "AdvisorQueue.h"
#include "CMessageCentre.h"
#include "CMsgDeath.h"
#include "Debug.h"
#include "FS_Online.h"
#include "LbFile.h"
#include "MainBalance.h"
#include "MapCell.h"
#include "ThingReceivers.h"
#include "World.h"
#include "MslSubscriptionTree.h"
#include "SaveLoad.h"
#include "analyser.h"

// 0x10152758: writes the count 0x1d (the Mac's big-endian swap of the word; the host's bytes 1d 00 00 00 are the same),
// then each header; stops at the first nonzero answer and returns it.
int32_t CMessageCentre::Save(void*& file) const {
    uint32_t count = 0x1d;
    uint32_t written;
    LbFile_Write(file, &count, 4, &written);
    for (int32_t i = 0; i < 0x1d; i++) {
        int32_t r = mSubscriptions[i].Save(file);
        if (r != 0) return r;
    }
    return 0;
}

// 0x10152840: clears all 29 trees, reads one word (ignored), then loads each header.
int32_t CMessageCentre::Load(void*& file) {
    for (int32_t i = 0; i < 0x1d; i++) mSubscriptions[i].mTree.clear();
    uint32_t value;
    uint32_t read;
    LbFile_Read(file, &value, 4, &read);
    // TPW_PORT_FIX v5L27 CMessageCentre::Load | Mac: reads each header's count and that many ids | Port: for a park an early port build wrote, reads the 29 counts alone and subscribes the loaded things as their constructors did | Why: those builds wrote the counts without the ids (see gPortEarlySave in SaveLoad.h)
    if (gPortEarlySave) {
        uint32_t counts[0x1d];
        for (int32_t i = 0; i < 0x1d; i++) {
            counts[i] = 0;
            LbFile_Read(file, &counts[i], 4, &read);
        }
        PortSubscribeLoadedThings(*this);
        for (int32_t i = 0; i < 0x1d; i++) {
            // A type the park had no subscriber for stays empty: the things of a shipped level keep the subscriptions of the
            // game that wrote it (Easymode's interface receiver has no 0x1c).
            if (counts[i] == 0) mSubscriptions[i].mTree.clear();
            if (mSubscriptions[i].mTree.mCount != counts[i])
                std::fprintf(stderr, "port: message type %d has %u subscriptions, the early park recorded %u\n", i,
                             mSubscriptions[i].mTree.mCount, counts[i]);
        }
        return 0;
    }
    for (int32_t i = 0; i < 0x1d; i++) {
        int32_t r = mSubscriptions[i].Load(file);
        if (r != 0) return r;
    }
    return 0;
}

// 0x10152448
int32_t CMessageCentre::UnsubscribeAll(const CThing& thing) {
    int32_t total = 0;
    for (int32_t i = 0; i < 0x1d; i++) {
        CSubscription key;
        key.mThingId = thing.GetThingId().mIndex;
        SubscriptionTree& tree = mSubscriptions[i].mTree;
        SubscriptionTree::node_base* it = tree.find(key);
        if (it != tree.end()) {
            tree.erase(it);
            total += 1;
        }
    }
    return total;
}

// TPW_PORT_FIX v5L27 PortSubscribeLoadedThings | Mac: no such function (the file holds the subscriptions) | Port: makes the subscriptions of every live thing as its constructor and Init did | Why: a park an early port build wrote has no subscription ids (see gPortEarlySave in SaveLoad.h)
void PortSubscribeLoadedThings(CMessageCentre& centre) {
    // The types by thing kind, -1 ending each row: a peep's constructor takes 10 (CPeep), a staff member's Init 12 (CStaff::Init),
    // with 10 again (CMechanic::Init, CHandyman::Init) or 0x1b (CGuard::Init); the others are their constructors' or Inits' lists.
    static const int8_t kTypes[0x14][9] = {
        {-1},                                    // 0
        {10, -1},                                // 1 CPerson
        {-1},                                    // 2
        {0xb, -1},                               // 3 CObject
        {10, 12, -1},                            // 4 CMechanic
        {10, 12, -1},                            // 5 CHandyman
        {10, 12, -1},                            // 6 CEntertainer
        {10, 12, 0x1b, -1},                      // 7 CGuard
        {10, 12, -1},                            // 8 CResearcher
        {0xc, 0xe, -1},                          // 9 CStaffHQ
        {7, -1},                                 // 10 CMechanicHQ
        {5, 17, 19, 23, 14, 8, 25, -1},          // 11 CAdvisor::Init
        {7, 12, 20, 24, 25, 19, -1},             // 12 CTagSystem::Init
        {0xc, 0x11, 0x13, -1},                   // 13 CParkAnalyser
        {0x11, -1},                              // 14 CResearchLab
        {1, 2, -1},                              // 15 CWeather
        {0xc, 0x13, 0xd, -1},                    // 16 CBankAccount
        {0x12, 0xa, 7, 8, 0x1b, 0xf, 0x13, 0x1c, -1},   // 17 CUIMsgReceiver
        {-1},                                    // 18 (an online person is not loaded)
        {0x1a, 0xb, 0x11, -1},                   // 19 CChallengeHQ
    };
    for (CThingArrayEntry* entry = gUsedThingHead; entry != nullptr; entry = entry->mNextUsed) {
        const CThing* thing = static_cast<const CThing*>(entry->mThing);
        if (thing == nullptr || thing->mDead != 0 || thing->mKind >= 0x14) continue;
        const int8_t* types = kTypes[thing->mKind];
        for (int32_t i = 0; i < 9 && types[i] >= 0; i++) centre.Subscribe(*thing, static_cast<MsgType>(types[i]));
    }
}

