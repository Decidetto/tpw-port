// resource_fork.h: a read-only parser for Macintosh resource forks (Inside Macintosh, "Resource
// Manager"), as the project's disc extractor writes them next to a file as "<name>.rsrc". The
// game's TrueType fonts on the data disc are 'sfnt' resources of such a fork: their data fork is
// empty. Plain C++ with no platform headers.
//
// Layout (all big-endian): the fork starts with the data offset, the map offset, the data length
// and the map length (four u32). Each resource's data is a u32 length followed by the bytes, at
// data offset + the entry's 3-byte offset. The map holds 24 bytes of copied header, a handle, a
// file reference and attributes (16 + 4 + 2 + 2), then the u16 offsets (from the map start) of
// the type list and of the name list. The type list is a u16 (count - 1) followed by entries of
// a 4-byte type, a u16 (count of that type - 1) and a u16 offset (from the type list start) of its
// reference list; a reference is a u16 id, a u16 name offset (0xffff: none), one byte of attributes
// with a 3-byte data offset, and 4 bytes of handle. A name is a length byte and the characters.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tpw {

// One resource of a fork. data points into the buffer the fork was parsed from (not a copy).
struct ForkResource {
    uint32_t type = 0;           // four characters, first one in the high byte ('sfnt' is 0x73666e74)
    int16_t id = 0;
    std::string name;            // Mac Roman bytes as stored; empty when the resource has none
    const uint8_t* data = nullptr;
    size_t size = 0;
};

constexpr uint32_t ForkType(const char (&code)[5]) {
    return (static_cast<uint32_t>(static_cast<uint8_t>(code[0])) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(code[1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(code[2])) << 8) | static_cast<uint32_t>(static_cast<uint8_t>(code[3]));
}

// Every resource of the fork in file order. Every offset and length is checked against the buffer
// size; a truncated, inconsistent or hostile fork answers an empty list (no partial answers).
std::vector<ForkResource> ListResources(const uint8_t* fork, size_t size);

// The resources of one type, in file order (empty when none or when the fork is refused).
std::vector<ForkResource> FindResources(const uint8_t* fork, size_t size, uint32_t type);

}  // namespace tpw
