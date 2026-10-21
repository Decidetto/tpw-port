#include "AdvisorScoring.h"
#include "AdvisorQueue.h"
#include "Serialise.h"

// LOADVARSTRUCT<AdvisorHistory> 0x1000ecb4 (260 B): the file's 16 bytes, then the words at +0, +4 and +0xc byte-reversed.
// The 0x10 byte record is AdvisorMessageState here.
static void UnswapForFile(AdvisorMessageState& state) {
    static_assert(sizeof(AdvisorMessageState) == 0x10, "the record is 16 bytes");
    state.mLastSaid.mTimeSig = serialise::UnswapWord(state.mLastSaid.mTimeSig);
    state.mResponseIndex = serialise::UnswapWord(state.mResponseIndex);
    state.mKillCount = serialise::UnswapWord(state.mKillCount);
}

template<> int32_t LOADVARSTRUCT<AdvisorMessageState>(void* file, AdvisorMessageState* value, const char* name, int32_t noLog) {
    // TPW_PORT_FIX v5L27 LOADVARSTRUCT<AdvisorMessageState> | Mac: reads the words at +0, +4 and +0xc little-endian | Port: reads them byte-reversed from a park an early port build wrote | Why: that build's SAVEVARSTRUCT reversed them (see gPortEarlySave in SaveLoad.h)
    int32_t result = serialise::LoadStruct(file, value, name, noLog);
    if (result == 0 && gPortEarlySave) {
        value->mLastSaid.mTimeSig = static_cast<decltype(value->mLastSaid.mTimeSig)>(
            __builtin_bswap32(static_cast<uint32_t>(value->mLastSaid.mTimeSig)));
        value->mResponseIndex = static_cast<decltype(value->mResponseIndex)>(__builtin_bswap32(static_cast<uint32_t>(value->mResponseIndex)));
        value->mKillCount = static_cast<decltype(value->mKillCount)>(__builtin_bswap32(static_cast<uint32_t>(value->mKillCount)));
    }
    return result;
}
