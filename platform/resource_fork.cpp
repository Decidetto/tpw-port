// resource_fork.cpp: see resource_fork.h. Every read goes through Bytes::at, which refuses a
// range that does not lie inside the buffer, and the parse gives up on the first refusal.
#include "resource_fork.h"

namespace tpw {

namespace {

// A bounds-checked view of the fork's bytes.
struct Bytes {
    const uint8_t* data;
    size_t size;

    // True when [offset, offset + length) lies inside the buffer (no overflow).
    bool has(uint64_t offset, uint64_t length) const { return offset <= size && length <= size - offset; }
    uint32_t u16(size_t offset) const { return static_cast<uint32_t>(data[offset] << 8 | data[offset + 1]); }
    uint32_t u32(size_t offset) const { return u16(offset) << 16 | u16(offset + 2); }
};

constexpr size_t kHeader = 16;
constexpr size_t kMapFixed = 28;     // 24 bytes of header copy, then the type list and name list offsets
constexpr size_t kTypeEntry = 8;
constexpr size_t kReference = 12;

}  // namespace

std::vector<ForkResource> ListResources(const uint8_t* fork, size_t size) {
    std::vector<ForkResource> found;
    if (fork == nullptr) return found;
    const Bytes bytes{fork, size};
    if (!bytes.has(0, kHeader)) return found;
    const uint64_t dataOffset = bytes.u32(0);
    const uint64_t mapOffset = bytes.u32(4);
    const uint64_t dataLength = bytes.u32(8);
    const uint64_t mapLength = bytes.u32(12);
    if (!bytes.has(dataOffset, dataLength) || !bytes.has(mapOffset, mapLength) || mapLength < kMapFixed) return found;
    // The map is the only place offsets are taken from; all of them must stay inside it.
    const uint64_t mapEnd = mapOffset + mapLength;
    const uint64_t typeList = mapOffset + bytes.u16(static_cast<size_t>(mapOffset) + 24);
    const uint64_t nameList = mapOffset + bytes.u16(static_cast<size_t>(mapOffset) + 26);
    if (typeList + 2 > mapEnd) return found;
    const uint32_t typeCount = (bytes.u16(static_cast<size_t>(typeList)) + 1) & 0xffff;   // 0xffff: no types
    if (typeList + 2 + static_cast<uint64_t>(typeCount) * kTypeEntry > mapEnd) return found;
    for (uint32_t t = 0; t < typeCount; ++t) {
        const size_t entry = static_cast<size_t>(typeList) + 2 + t * kTypeEntry;
        const uint32_t type = bytes.u32(entry);
        const uint32_t count = bytes.u16(entry + 4) + 1;
        const uint64_t references = typeList + bytes.u16(entry + 6);
        if (references + static_cast<uint64_t>(count) * kReference > mapEnd) return {};
        for (uint32_t r = 0; r < count; ++r) {
            const size_t reference = static_cast<size_t>(references) + r * kReference;
            ForkResource resource;
            resource.type = type;
            resource.id = static_cast<int16_t>(bytes.u16(reference));
            const uint32_t nameOffset = bytes.u16(reference + 2);
            if (nameOffset != 0xffff) {
                const uint64_t at = nameList + nameOffset;
                if (at + 1 > mapEnd) return {};
                const size_t length = fork[at];
                if (at + 1 + length > mapEnd) return {};
                resource.name.assign(reinterpret_cast<const char*>(fork + at + 1), length);
            }
            const uint64_t offset = bytes.u32(reference + 4) & 0xffffff;
            if (offset + 4 > dataLength) return {};
            const uint64_t length = bytes.u32(static_cast<size_t>(dataOffset + offset));
            if (offset + 4 + length > dataLength) return {};
            resource.data = fork + dataOffset + offset + 4;
            resource.size = static_cast<size_t>(length);
            found.push_back(std::move(resource));
        }
    }
    return found;
}

std::vector<ForkResource> FindResources(const uint8_t* fork, size_t size, uint32_t type) {
    std::vector<ForkResource> found;
    for (ForkResource& resource : ListResources(fork, size)) {
        if (resource.type == type) found.push_back(std::move(resource));
    }
    return found;
}

}  // namespace tpw
