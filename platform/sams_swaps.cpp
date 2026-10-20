// sams_swaps.cpp: the SamsUtilities byte-swap instantiations the ride height map loader calls
// by their Mac addresses (include/TPEngine.h: USwapInplace32<unsigned long> 0x1005e658,
// USwapInplace16<unsigned short> 0x1005e610, USwap32<unsigned char*> 0x1005e5b4). They
// convert little-endian file data to the Mac's byte order; on a little-endian host the data
// is already the right way round, so like _USwapBlock32 (sams_core.cpp) they leave it alone.
// Nothing here is a translation of game code.
#include <cstdint>

#include "TPEngine.h"

namespace {
bool HostIsLittleEndian() {
    const uint16_t probe = 1;
    return *reinterpret_cast<const uint8_t*>(&probe) == 1;
}
}  // namespace

void USwapInplace32ULong_1005e658(uint32_t& word) {
    if (HostIsLittleEndian()) return;
    word = (word >> 24) | ((word >> 8) & 0xff00u) | ((word << 8) & 0xff0000u) | (word << 24);
}

void USwapInplace16UShort_1005e610(uint16_t& half) {
    if (HostIsLittleEndian()) return;
    half = static_cast<uint16_t>(half >> 8 | half << 8);
}

uint32_t USwap32UCharPtr_1005e5b4(uint32_t word) {
    if (HostIsLittleEndian()) return word;
    return (word >> 24) | ((word >> 8) & 0xff00u) | ((word << 8) & 0xff0000u) | (word << 24);
}
