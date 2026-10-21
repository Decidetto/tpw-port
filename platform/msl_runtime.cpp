// msl_runtime.cpp: the Metrowerks runtime imports the translated library code calls through
// include/ui_ime.h (operator new and delete, the string and wide-memory primitives, __throw),
// on the host's heap and C library. The unit tests fake these; the launcher had stand-ins
// that returned 0, so the first std::map insertion of a level (CMessageCentre::Subscribe
// from CStaffHQ's constructor) wrote through a NULL node pointer.
//
// Sizes: a translation passes the byte count the Mac code passed (0xe for a tree node, 0x10
// for a string representation, 12 for a list node, mBlockSize * 0xc for a deque block) and
// then uses the memory as the host's layout, whose pointers are twice as wide. Every request
// is therefore granted twice its size plus a margin, and zeroed, so the host object always
// fits whichever size was asked for. Nothing here is a translation of game code.
#include <cstdint>
#include <cstring>
#include <new>

#include "platform.h"
#include "platform/msl.h"
#include "ui_ime.h"

namespace {

// The host size a Mac request `size` may need: every 4 byte word can be an 8 byte pointer.
uint32_t HostSize(uint32_t size) {
    const uint32_t kMinimum = 64;
    uint64_t wanted = static_cast<uint64_t>(size) * 2u + 32u;
    if (wanted < kMinimum) wanted = kMinimum;
    if (wanted > 0xffffffffu) wanted = 0xffffffffu;
    return static_cast<uint32_t>(wanted);
}

void* Allocate(uint32_t size) {
    void* p = ::operator new(HostSize(size), std::nothrow);
    if (p != nullptr) std::memset(p, 0, HostSize(size));
    return p;
}

}  // namespace

namespace IME {

size_t MslStrlen(const char* s) { return s == nullptr ? 0 : std::strlen(s); }

char* MslStrcpy(char* d, const char* s) {
    if (d == nullptr || s == nullptr) return d;
    return std::strcpy(d, s);
}

void* MslNew(uint32_t size) { return Allocate(size); }

void* MslNewArray(uint32_t size) { return Allocate(size); }

void MslDelete(void* p) { ::operator delete(p); }

void MslDeleteArray(void* p) { ::operator delete(p); }

// operator new(0x10) for a string representation: the host MslCharRep and MslWideStringRep are
// both 24 bytes (three words and a pointer); the general rule grants 64, which covers either.
void* MslNewRep() { return Allocate(0x10); }

// __throw: the unit tests' fakes return, and the translations were checked on that behaviour,
// so the launcher logs the exception and returns the same way. `object` is the library's
// exception object (vtable word, then the reference-counted message).
void MslThrow(const char* typeName, void* object, const void* /*dtorTV*/) {
    const char* message = "";
    if (object != nullptr) {
        const MslExceptionObject* exception = static_cast<const MslExceptionObject*>(object);
        if (exception->mRc.mPtr != nullptr) message = exception->mRc.mPtr;
    }
    tpw::Log("msl: throw %s: %s (not propagated)", typeName == nullptr ? "?" : typeName, message);
}

int32_t MslWmemcmp(const char16_t* a, const char16_t* b, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

char16_t* MslWmemcpy(char16_t* d, const char16_t* s, uint32_t n) {
    if (n != 0) std::memcpy(d, s, static_cast<size_t>(n) * sizeof(char16_t));
    return d;
}

char16_t* MslWmemset(char16_t* d, char16_t c, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i) d[i] = c;
    return d;
}

char16_t* MslWmemmove(char16_t* d, const char16_t* s, uint32_t n) {
    if (n != 0) std::memmove(d, s, static_cast<size_t>(n) * sizeof(char16_t));
    return d;
}

}  // namespace IME

// rand of the C library (0x10239148), which the level setup calls for the park's seed
// (src/game/rides/BumperRide.cpp declares it as ::Rand): the one generator the MSL
// rand and srand of platform/msl.cpp drive, so the game's seeding applies to it.
int32_t Rand();
int32_t Rand() { return msl_rand(); }
