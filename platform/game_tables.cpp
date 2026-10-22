// game_tables.cpp: see include/platform/game_tables.h. The reader mirrors devtools/gen_game_tables.py step for step (the
// PEF container, the pattern-initialised data section and its relocations, the view of another build at the 2.04b3
// build's addresses, and the seven tables' field reads), so the tables it fills are byte for byte the arrays that script
// writes for a configure-time build (platform/tests/game_tables_tests.cpp compares them). Nothing of the tables is in
// this file: the lists it compiles in (include/platform/game_table_lists.inc) are names, addresses, counts, offsets and
// SHA-1s. Every read is bounds checked; a refusal ends the load with the reason, never with a crash.
#include "platform/game_tables.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>

#include "Advisor.h"
#include "AdvisorScoring.h"
#include "BMP_InsideArea.h"
#include "CBalanceTable.h"
#include "SaveLoad.h"

#ifndef TPW_TABLES_AT_CONFIGURE
#define TPW_TABLES_AT_CONFIGURE 0
#endif

namespace tpw {

namespace {

// ---- SHA-1 (FIPS 180-4), for the program's identity and the blocks' checks

struct Sha1Digest {
    uint8_t bytes[20];
    bool operator==(const Sha1Digest& other) const { return std::memcmp(bytes, other.bytes, 20) == 0; }
    bool operator<(const Sha1Digest& other) const { return std::memcmp(bytes, other.bytes, 20) < 0; }
};

uint32_t Rotl(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

void Sha1Block(uint32_t h[5], const uint8_t* block) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
        w[i] = uint32_t(block[4 * i]) << 24 | uint32_t(block[4 * i + 1]) << 16 | uint32_t(block[4 * i + 2]) << 8 | block[4 * i + 3];
    for (int i = 16; i < 80; i++) w[i] = Rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        const uint32_t t = Rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = Rotl(b, 30);
        b = a;
        a = t;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

Sha1Digest Sha1(const uint8_t* data, size_t size) {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    size_t done = 0;
    for (; size - done >= 64; done += 64) Sha1Block(h, data + done);
    uint8_t tail[128] = {};
    const size_t rest = size - done;
    std::memcpy(tail, data + done, rest);
    tail[rest] = 0x80;
    const size_t blocks = rest + 9 <= 64 ? 1 : 2;
    const uint64_t bits = uint64_t(size) * 8;
    for (int i = 0; i < 8; i++) tail[blocks * 64 - 1 - i] = uint8_t(bits >> (8 * i));
    for (size_t i = 0; i < blocks; i++) Sha1Block(h, tail + 64 * i);
    Sha1Digest out;
    for (int i = 0; i < 20; i++) out.bytes[i] = uint8_t(h[i / 4] >> (24 - 8 * (i % 4)));
    return out;
}

Sha1Digest Sha1FromHex(const char* hex) {
    Sha1Digest out = {};
    for (int i = 0; i < 40 && hex[i]; i++) {
        const char ch = hex[i];
        const int v = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ch - 'A' + 10;
        out.bytes[i / 2] = uint8_t(out.bytes[i / 2] << 4 | v);
    }
    return out;
}

// ---- the lists (include/platform/game_table_lists.inc)

struct PartInfo {
    const char* name;
    uint32_t address;   // in the 2.04b3 build
    uint32_t length;
    const char* anchor;
    const char* whole;
};

enum PartId {
#define TPW_GAME_TABLE_PART(id, address, length, anchor, whole) kPart##id,
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_PART
    kPartCount
};

const PartInfo kParts[kPartCount] = {
#define TPW_GAME_TABLE_PART(id, address, length, anchor, whole) {#id, address, length, anchor, whole},
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_PART
};

struct ScriptInfo {
    uint32_t address;   // in the 2.04b3 build
    int words;
    const char* array;
    const char* anchor;
    const char* whole;
    int ordinal;        // its place among the identical copies, by address
    int copies;         // the number of identical copies in the 2.04b3 build
};

const ScriptInfo kScripts[] = {
#define TPW_GAME_TABLE_WINDOW_SCRIPT(address, words, array, anchor, whole, ordinal, copies) \
    {0x##address, words, #array, anchor, whole, ordinal, copies},
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_WINDOW_SCRIPT
};
static_assert(sizeof(kScripts) / sizeof(kScripts[0]) == kGameTableWindowScripts, "game_table_lists.inc lists other scripts");

struct ScorerInfo {
    uint32_t address;   // the 2.04b3 code address include/AdvisorScoring.h declares
    AdvisorScorer scorer;
};

const ScorerInfo kScorers[] = {
#define TPW_GAME_TABLE_SCORER(address, name) {address, &CAdvisorScoring::name},
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_SCORER
};
static_assert(sizeof(kScorers) / sizeof(kScorers[0]) == kGameTableScorers, "game_table_lists.inc lists other scorers");

struct BuildInfo {
    const char* name;
    const char* sha1;   // of the data fork
    uint32_t size;
    int32_t parts[kPartCount];                    // offsets from the 2.04b3 addresses
    int32_t scripts[kGameTableWindowScripts];
};

#define TPW_UNPAREN(...) __VA_ARGS__
const BuildInfo kBuilds[] = {
#define TPW_GAME_TABLE_BUILD(name, sha1, size, parts, scripts) {name, sha1, size, {TPW_UNPAREN parts}, {TPW_UNPAREN scripts}},
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_BUILD
};
#undef TPW_UNPAREN

constexpr uint32_t kCodeBase = 0x10000000;
constexpr uint32_t kImportsBase = 0x103BC8B0;
constexpr uint32_t kAnchor = 32;                                    // bytes hashed to find a block
constexpr uint32_t kScorerTable = 0x1027DF30;                       // 351 pointers to member of 12 bytes
constexpr uint32_t kMessages = 0x1027EFA4;
constexpr uint32_t kTVectorsFirst = 0x10265800, kTVectorsEnd = 0x10265F8C;   // the scorers' 241 TVectors in 2.04b3

bool Fail(std::string* why, const std::string& text) {
    if (why) *why = text;
    return false;
}

std::string Hex(uint32_t v) {
    char text[16];
    std::snprintf(text, sizeof text, "0x%08x", v);
    return text;
}

uint32_t U32(const uint8_t* b) { return uint32_t(b[0]) << 24 | uint32_t(b[1]) << 16 | uint32_t(b[2]) << 8 | b[3]; }
uint32_t U16(const uint8_t* b) { return uint32_t(b[0]) << 8 | b[1]; }

// ---- the PEF container: the code section as stored, the data section unpacked and relocated (Program)

struct Program {
    std::vector<uint8_t> code;
    std::vector<uint8_t> data;
    uint32_t dataBase = 0;

    // The size bytes at addr, from the code section or else the data section; null outside both.
    const uint8_t* At(uint64_t addr, uint64_t size) const {
        if (kCodeBase <= addr && addr + size <= kCodeBase + uint64_t(code.size())) return code.data() + (addr - kCodeBase);
        if (dataBase <= addr && addr + size <= dataBase + uint64_t(data.size())) return data.data() + (addr - dataBase);
        return nullptr;
    }
};

// A bounds-checked reader of the file's bytes.
struct Bytes {
    const uint8_t* data;
    size_t size;
    bool has(uint64_t offset, uint64_t length) const { return offset <= size && length <= size - offset; }
};

// The variable-length argument of a pidata opcode.
bool PidataArg(const Bytes& b, size_t* p, uint64_t* v) {
    *v = 0;
    for (;;) {
        if (*p >= b.size || *v > (uint64_t(1) << 40)) return false;
        const uint8_t c = b.data[(*p)++];
        *v = (*v << 7) | (c & 0x7F);
        if (!(c & 0x80)) return true;
    }
}

bool UnpackPidata(const Bytes& b, uint32_t size, std::vector<uint8_t>* out, std::string* why) {
    out->clear();
    size_t p = 0;
    auto copy = [&](uint64_t count) {
        if (!b.has(p, count) || out->size() + count > size) return false;
        out->insert(out->end(), b.data + p, b.data + p + count);
        p += count;
        return true;
    };
    auto zeros = [&](uint64_t count) {
        if (out->size() + count > size) return false;
        out->insert(out->end(), count, 0);
        return true;
    };
    while (p < b.size) {
        const int op = b.data[p] >> 5;
        uint64_t count = b.data[p] & 0x1F;
        p++;
        if (count == 0 && !PidataArg(b, &p, &count)) return Fail(why, "the data section's packing is cut short");
        bool ok = true;
        if (op == 0) {                                  // zero
            ok = zeros(count);
        } else if (op == 1) {                           // block copy
            ok = copy(count);
        } else if (op == 2) {                           // repeated block
            uint64_t rep;
            ok = PidataArg(b, &p, &rep) && b.has(p, count) && (rep + 1) * count <= size;
            for (uint64_t r = 0; ok && r <= rep; r++) {
                const size_t at = p;
                ok = copy(count);
                p = at;
            }
            p += count;
        } else if (op == 3 || op == 4) {                // a common block (op 3) or zeros (op 4) interleaved with copied blocks
            uint64_t custom, rep;
            ok = PidataArg(b, &p, &custom) && PidataArg(b, &p, &rep);
            const size_t common = p;
            if (ok && op == 3) ok = copy(count);
            if (ok && op == 4) ok = zeros(count);
            for (uint64_t r = 0; ok && r < rep; r++) {
                ok = copy(custom);
                if (ok && op == 3) {
                    const size_t at = p;
                    p = common;
                    ok = copy(count);
                    p = at;
                }
                if (ok && op == 4) ok = zeros(count);
            }
        } else {
            return Fail(why, "the data section has pidata opcode " + std::to_string(op));
        }
        if (!ok) return Fail(why, "the data section's packing does not fit its size");
    }
    out->resize(size, 0);
    return true;
}

// The loader section's relocations of the data section, as the Code Fragment Manager applies them.
class Relocator {
public:
    Relocator(Program* program, const uint32_t bases[2], int codeIndex, int dataIndex)
        : mData(program->data), mCodeIndex(codeIndex), mDataIndex(dataIndex) {
        mBase[0] = bases[0];
        mBase[1] = bases[1];
    }

    bool Run(const std::vector<uint16_t>& ins, std::string* why) {
        mIns = &ins;
        mPos = 0;
        mImport = 0;
        mC = mBase[0];
        mD = mBase[1];
        for (size_t k = 0; k < ins.size() && mError.empty();) k += Step(k, 0);
        if (!mError.empty()) return Fail(why, mError);
        return true;
    }

private:
    std::vector<uint8_t>& mData;
    int mCodeIndex, mDataIndex;
    uint32_t mBase[2];
    const std::vector<uint16_t>* mIns = nullptr;
    uint64_t mPos = 0;
    uint32_t mImport = 0, mC = 0, mD = 0;
    std::string mError;

    bool SectionBase(uint32_t index, uint32_t* base) {
        if (int(index) == mCodeIndex) *base = mBase[0];
        else if (int(index) == mDataIndex) *base = mBase[1];
        else return Error("a relocation names section " + std::to_string(index));
        return true;
    }

    bool Error(const std::string& text) {
        if (mError.empty()) mError = text;
        return false;
    }

    void Word(uint32_t v) {
        if (mPos + 4 > mData.size()) {
            Error("a relocation lies outside the data section");
            return;
        }
        uint8_t* p = mData.data() + mPos;
        const uint32_t w = U32(p) + v;
        p[0] = uint8_t(w >> 24);
        p[1] = uint8_t(w >> 16);
        p[2] = uint8_t(w >> 8);
        p[3] = uint8_t(w);
        mPos += 4;
    }

    void Repeat(size_t k, size_t blocks, uint32_t times, int depth) {
        if (blocks > k) {
            Error("a relocation repeat reaches before the first instruction");
            return;
        }
        for (uint32_t t = 0; t < times && mError.empty(); t++)
            for (size_t j = k - blocks; j < k && mError.empty();) j += Step(j, depth + 1);
    }

    // Runs the instruction at k; answers its length in halfwords.
    size_t Step(size_t k, int depth) {
        const std::vector<uint16_t>& ins = *mIns;
        if (depth > 8) {
            Error("relocation repeats nest too deeply");
            return 1;
        }
        const uint32_t h = ins[k];
        if (h >> 14 == 0) {                                     // by section D, after a skip
            mPos += 4 * ((h >> 6) & 0xFF);
            for (uint32_t i = 0; i < (h & 0x3F) && mError.empty(); i++) Word(mD);
            return 1;
        }
        if (h >> 13 == 0b010) {                                 // runs
            const uint32_t sub = (h >> 9) & 0xF;
            for (uint32_t i = 0; i < (h & 0x1FF) + 1 && mError.empty(); i++) {
                if (sub == 0) {
                    Word(mC);
                } else if (sub == 1) {
                    Word(mD);
                } else if (sub == 2 || sub == 3) {              // transition vectors of 12 and 8 bytes
                    const uint64_t at = mPos;
                    Word(mC);
                    Word(mD);
                    mPos = at + (sub == 2 ? 12 : 8);
                } else if (sub == 4) {                          // virtual table entries of 8 bytes
                    Word(mD);
                    mPos += 4;
                } else if (sub == 5) {                          // a run of imports
                    Word(kImportsBase + 4 * mImport);
                    mImport++;
                } else {
                    Error("relocation run " + std::to_string(sub));
                }
            }
            return 1;
        }
        if (h >> 13 == 0b011) {                                 // small index forms
            const uint32_t sub = (h >> 9) & 0xF, idx = h & 0x1FF;
            uint32_t base = 0;
            if (sub == 0) {
                Word(kImportsBase + 4 * idx);
                mImport = idx + 1;
            } else if (sub == 1) {
                if (SectionBase(idx, &base)) mC = base;
            } else if (sub == 2) {
                if (SectionBase(idx, &base)) mD = base;
            } else if (sub == 3) {
                if (SectionBase(idx, &base)) Word(base);
            } else {
                Error("relocation small index " + std::to_string(sub));
            }
            return 1;
        }
        if (h >> 12 == 0b1000) {                                // advance
            mPos += (h & 0xFFF) + 1;
            return 1;
        }
        if (h >> 12 == 0b1001) {                                // small repeat
            Repeat(k, ((h >> 8) & 0xF) + 1, (h & 0xFF) + 1, depth);
            return 1;
        }
        if (k + 1 >= ins.size()) {
            Error("a relocation instruction is cut short");
            return 2;
        }
        const uint32_t arg = ((h & 0x3FF) << 16) | ins[k + 1];
        const uint32_t arg6 = ((h & 0x3F) << 16) | ins[k + 1];
        if (h >> 10 == 0b101000) {                              // set position
            mPos = arg;
        } else if (h >> 10 == 0b101001) {                       // by import, large index
            Word(kImportsBase + 4 * arg);
            mImport = arg + 1;
        } else if (h >> 10 == 0b101100) {                       // large repeat
            Repeat(k, ((h >> 6) & 0xF) + 1, arg6 + 1, depth);
        } else if (h >> 10 == 0b101101) {                       // by or set section, large index
            const uint32_t sub = (h >> 6) & 0xF;
            uint32_t base = 0;
            if (sub > 2) Error("relocation large section " + std::to_string(sub));
            else if (SectionBase(arg6, &base)) {
                if (sub == 0) Word(base);
                else if (sub == 1) mC = base;
                else mD = base;
            }
        } else {
            Error("relocation opcode " + Hex(h));
        }
        return 2;
    }
};

bool ParseProgram(const Bytes& f, Program* p, std::string* why) {
    if (f.size < 40 || std::memcmp(f.data, "Joy!peffpwpc", 12) != 0)
        return Fail(why, "it is not a PowerPC PEF container (give the data fork of the application)");
    struct Section {
        uint32_t total, unpacked, kind;
        Bytes raw;
    };
    std::vector<Section> sections;
    const uint32_t count = U16(f.data + 0x20);
    for (uint32_t i = 0; i < count; i++) {
        const size_t at = 40 + 28 * size_t(i);
        if (!f.has(at, 25)) return Fail(why, "its section headers are cut short");
        const uint32_t length = U32(f.data + at + 16), offset = U32(f.data + at + 20);
        if (!f.has(offset, length)) return Fail(why, "a section lies outside the file");
        sections.push_back({U32(f.data + at + 8), U32(f.data + at + 12), f.data[at + 24], {f.data + offset, length}});
    }
    int codeIndex = -1, dataIndex = -1, loaderIndex = -1;
    for (int i = 0; i < int(sections.size()); i++) {
        const uint32_t kind = sections[i].kind;
        if (codeIndex < 0 && (kind == 0 || kind == 3)) codeIndex = i;
        if (dataIndex < 0 && (kind == 1 || kind == 2)) dataIndex = i;
        if (loaderIndex < 0 && kind == 4) loaderIndex = i;
    }
    if (codeIndex < 0 || dataIndex < 0 || loaderIndex < 0) return Fail(why, "it has no code, data or loader section");
    const Section& code = sections[codeIndex];
    const Section& data = sections[dataIndex];
    p->code.assign(code.raw.data, code.raw.data + code.raw.size);
    p->dataBase = (kCodeBase + code.total + 15) & ~15u;
    if (data.kind == 2) {
        if (!UnpackPidata(data.raw, data.unpacked, &p->data, why)) return false;
    } else {
        if (data.raw.size > data.unpacked) return Fail(why, "the data section is larger than its unpacked size");
        p->data.assign(data.raw.data, data.raw.data + data.raw.size);
        p->data.resize(data.unpacked, 0);
    }
    // The loader section: the relocation headers follow the libraries and the imported symbols.
    const Bytes L = sections[loaderIndex].raw;
    if (!L.has(24, 16)) return Fail(why, "the loader section is cut short");
    const uint64_t nlibs = U32(L.data + 24), nsyms = U32(L.data + 28), nrel = U32(L.data + 32), instrOffset = U32(L.data + 36);
    const uint64_t headers = 56 + 24 * nlibs + 4 * nsyms;
    const uint32_t bases[2] = {kCodeBase, p->dataBase};
    Relocator relocator(p, bases, codeIndex, dataIndex);
    for (uint64_t r = 0; r < nrel; r++) {
        const uint64_t at = headers + 12 * r;
        if (!L.has(at, 12)) return Fail(why, "the relocation headers are cut short");
        const uint32_t sect = U16(L.data + at), n = U32(L.data + at + 4), first = U32(L.data + at + 8);
        if (int(sect) != dataIndex) continue;
        if (!L.has(instrOffset + first, 2 * uint64_t(n))) return Fail(why, "the relocations are cut short");
        std::vector<uint16_t> ins(n);
        for (uint32_t i = 0; i < n; i++) ins[i] = uint16_t(U16(L.data + instrOffset + first + 2 * i));
        if (!relocator.Run(ins, why)) return false;
    }
    return true;
}

// ---- another build, read at the 2.04b3 build's addresses (gen_game_tables.py's BuildView)

struct Region {
    uint32_t lo, hi;
    int64_t delta;
};

class View {
public:
    explicit View(const Program& p) : mP(p) {}

    std::vector<Region> regions;
    std::unordered_map<uint32_t, uint32_t> patch;   // 2.04b3 word address -> the word the tables must see there

    // The n bytes at the 2.04b3 address a, from the first region that holds them (unpatched).
    const uint8_t* Raw(uint32_t a, uint32_t n) const {
        for (const Region& r : regions)
            if (r.lo <= a && uint64_t(a) + n <= r.hi) {
                const int64_t at = int64_t(a) + r.delta;
                return at < 0 ? nullptr : mP.At(uint64_t(at), n);
            }
        return nullptr;
    }

    // The same with the patched words laid over them.
    bool Read(uint32_t a, uint32_t n, uint8_t* out, std::string* why) const {
        const uint8_t* raw = Raw(a, n);
        if (!raw) return Fail(why, Hex(a) + "+" + std::to_string(n) + " is outside the blocks found in the program");
        std::memcpy(out, raw, n);
        for (uint64_t w = a - a % 4; w < uint64_t(a) + n; w += 4) {
            const auto it = patch.find(uint32_t(w));
            if (it == patch.end()) continue;
            const uint8_t v[4] = {uint8_t(it->second >> 24), uint8_t(it->second >> 16), uint8_t(it->second >> 8),
                                  uint8_t(it->second)};
            for (uint64_t i = 0; i < 4; i++)
                if (a <= w + i && w + i < uint64_t(a) + n) out[w + i - a] = v[i];
        }
        return true;
    }

    bool Word(uint32_t a, uint32_t* v, std::string* why) const {
        uint8_t b[4];
        if (!Read(a, 4, b, why)) return false;
        *v = U32(b);
        return true;
    }

private:
    const Program& mP;
};

// The scan of gen_game_tables.py's find_blocks: every even offset of the two sections, the SHA-1 of its first kAnchor
// bytes looked up among the wanted blocks' anchors, then the whole block's SHA-1. hits[i]: the addresses of wanted[i].
struct Wanted {
    uint32_t length;
    Sha1Digest anchor, whole;
};

std::vector<std::vector<uint32_t>> FindBlocks(const Program& p, const std::vector<Wanted>& wanted) {
    std::vector<std::pair<Sha1Digest, size_t>> anchors;
    for (size_t i = 0; i < wanted.size(); i++) anchors.push_back({wanted[i].anchor, i});
    std::sort(anchors.begin(), anchors.end(), [](const auto& x, const auto& y) { return x.first < y.first || (x.first == y.first && x.second < y.second); });
    std::vector<std::vector<uint32_t>> hits(wanted.size());
    const std::pair<uint32_t, const std::vector<uint8_t>*> sections[2] = {{kCodeBase, &p.code}, {p.dataBase, &p.data}};
    for (const auto& section : sections) {
        const std::vector<uint8_t>& buf = *section.second;
        for (size_t i = 0; i + kAnchor <= buf.size(); i += 2) {
            const Sha1Digest anchor = Sha1(buf.data() + i, kAnchor);
            auto it = std::lower_bound(anchors.begin(), anchors.end(), anchor,
                                       [](const auto& x, const Sha1Digest& d) { return x.first < d; });
            for (; it != anchors.end() && it->first == anchor; ++it) {
                const Wanted& w = wanted[it->second];
                if (i + w.length <= buf.size() && Sha1(buf.data() + i, w.length) == w.whole)
                    hits[it->second].push_back(uint32_t(section.first + i));
            }
        }
    }
    for (auto& h : hits) std::sort(h.begin(), h.end());
    return hits;
}

// The view's regions for the parts and scripts: from a known build's offsets, or from the content search.
bool PlaceBlocks(const Program& p, const BuildInfo* known, View* view, std::string* why) {
    if (known) {
        for (int i = 0; i < kPartCount; i++)
            view->regions.push_back({kParts[i].address, kParts[i].address + kParts[i].length, known->parts[i]});
        for (int i = 0; i < kGameTableWindowScripts; i++) {
            const uint32_t lo = kScripts[i].address;
            view->regions.push_back({lo, lo + 2 * uint32_t(kScripts[i].words), known->scripts[i]});
        }
        // The program's SHA-1 already says what it holds; each block is checked whole all the same.
        for (size_t i = 0; i < view->regions.size(); i++) {
            const Region& r = view->regions[i];
            const char* whole = i < size_t(kPartCount) ? kParts[i].whole : kScripts[i - kPartCount].whole;
            const uint8_t* at = r.lo + r.delta < 0 ? nullptr : p.At(uint64_t(r.lo + r.delta), r.hi - r.lo);
            if (!at || !(Sha1(at, r.hi - r.lo) == Sha1FromHex(whole)))
                return Fail(why, std::string("the block at ") + Hex(r.lo) + " is not where its build holds it");
        }
    } else {
        std::vector<Wanted> wanted;
        for (const PartInfo& part : kParts) wanted.push_back({part.length, Sha1FromHex(part.anchor), Sha1FromHex(part.whole)});
        for (const ScriptInfo& s : kScripts)
            wanted.push_back({2 * uint32_t(s.words), Sha1FromHex(s.anchor), Sha1FromHex(s.whole)});
        const std::vector<std::vector<uint32_t>> hits = FindBlocks(p, wanted);
        for (int i = 0; i < kPartCount; i++) {
            if (hits[i].size() != 1)
                return Fail(why, std::string("the ") + kParts[i].name + " (" + std::to_string(kParts[i].length) +
                                     " bytes) are found " + std::to_string(hits[i].size()) + " times in the program, not once");
            view->regions.push_back({kParts[i].address, kParts[i].address + kParts[i].length,
                                     int64_t(hits[i][0]) - int64_t(kParts[i].address)});
        }
        for (int i = 0; i < kGameTableWindowScripts; i++) {
            const ScriptInfo& s = kScripts[i];
            const std::vector<uint32_t>& h = hits[kPartCount + i];
            uint32_t at;
            if (h.size() == 1 && s.copies == 1) at = h[0];
            else if (int(h.size()) == s.copies && s.copies > 1) at = h[s.ordinal];   // identical copies: the same place among them
            else
                return Fail(why, "the window script at " + Hex(s.address) + " is found " + std::to_string(h.size()) +
                                     " times in the program, not " + std::to_string(s.copies));
            view->regions.push_back({s.address, s.address + 2 * uint32_t(s.words), int64_t(at) - int64_t(s.address)});
        }
    }
    // The smallest block first, so a script wins over a wider block (a stable sort, as Python's).
    std::stable_sort(view->regions.begin(), view->regions.end(),
                     [](const Region& x, const Region& y) { return x.hi - x.lo < y.hi - y.lo; });
    return true;
}

// The scorer table: 351 pointers to member {0, -1, TVector}; the TVectors are consecutive in the 241-scorer region, and
// each scorer function is named by its rank among the 241 code addresses.
bool FollowScorers(const Program& p, View* view, std::string* why) {
    int64_t messageDelta = 0;
    bool found = false;
    for (const Region& r : view->regions)
        if (r.lo == kMessages) {
            messageDelta = r.delta;
            found = true;
            break;
        }
    if (!found) return Fail(why, "the message table is not placed");
    view->regions.push_back({kScorerTable, kMessages, messageDelta});   // record -1 is its last 0x30 bytes
    std::vector<uint32_t> entries;
    for (int k = 0; k < kGameTableMessageRecords; k++) {
        const uint8_t* b = view->Raw(kScorerTable + 12 * k, 12);
        if (!b) return Fail(why, "the scorer table is outside the program");
        if (U32(b) != 0 || U32(b + 4) != 0xFFFFFFFF)
            return Fail(why, "scorer " + std::to_string(k) + " is not a plain pointer to member");
        entries.push_back(U32(b + 8));
    }
    std::vector<uint32_t> distinct = entries;
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    bool consecutive = distinct.size() == size_t(kGameTableScorers);
    for (size_t i = 1; consecutive && i < distinct.size(); i++) consecutive = distinct[i] - distinct[i - 1] == 8;
    if (!consecutive)
        return Fail(why, "the scorer table names " + std::to_string(distinct.size()) + " TVectors, not " +
                             std::to_string(kGameTableScorers) + " consecutive ones");
    const int64_t tvDelta = int64_t(distinct[0]) - kTVectorsFirst;
    if (int64_t(distinct.back()) - tvDelta + 8 > int64_t(kTVectorsEnd) + 4)
        return Fail(why, "the scorers' TVectors end at an unexpected address");
    view->regions.push_back({kTVectorsFirst, kTVectorsEnd, tvDelta});
    std::vector<uint32_t> codes;
    for (uint32_t tv : entries) {
        const uint8_t* b = p.At(tv, 4);
        if (!b) return Fail(why, "a scorer's TVector " + Hex(tv) + " is outside the program");
        codes.push_back(U32(b));
    }
    std::sort(codes.begin(), codes.end());
    codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    if (codes.size() != size_t(kGameTableScorers))
        return Fail(why, std::to_string(codes.size()) + " scorer functions in the program, " + std::to_string(kGameTableScorers) +
                             " declared");
    for (int k = 0; k < kGameTableMessageRecords; k++) {
        const uint32_t tv = entries[k];
        const uint32_t code = U32(p.At(tv, 4));
        const size_t rank = size_t(std::lower_bound(codes.begin(), codes.end(), code) - codes.begin());
        view->patch[kScorerTable + 12 * k + 8] = uint32_t(int64_t(tv) - tvDelta);
        view->patch[uint32_t(int64_t(tv) - tvDelta)] = kScorers[rank].address;
    }
    return true;
}

// ---- the tables, read as gen_game_tables.py reads them

int32_t S32(uint32_t v) { return int32_t(v); }

bool ReadWords(const View& v, uint32_t a, uint32_t count, uint32_t* out, std::string* why) {
    std::vector<uint8_t> b(4 * size_t(count));
    if (!v.Read(a, 4 * count, b.data(), why)) return false;
    for (uint32_t i = 0; i < count; i++) out[i] = U32(b.data() + 4 * i);
    return true;
}

bool AdvisorLines(const View& v, AdvisorLine* lines, std::string* why) {
    static_assert(sizeof(AdvisorLine) == 0x20, "AdvisorLine is eight words");
    constexpr uint32_t kTable = 0x10278CE4;
    for (int r = 0; r < kGameTableAdvisorLines; r++) {
        uint32_t w[8];
        if (!ReadWords(v, kTable + 0x20 * r, 8, w, why)) return false;
        if ((w[0] == 9999) != (r == kGameTableAdvisorLines - 1))
            return Fail(why, "the response table's terminator row is not where expected");
        AdvisorLine& line = lines[r];
        line.mId = S32(w[0]);
        line.mSound = S32(w[1]);
        line.mLipFile = S32(w[2]);
        line.mSequence = S32(w[3]);
        line.mWhichAndSoundSet = w[4];
        line.mHat = S32(w[5]);
        line.mHand = S32(w[6]);
        line.mTag = S32(w[7]);
    }
    return true;
}

void SetRecord(AdvisorMessageRecord* r, const uint32_t w[12]) {
    r->mSkipped = S32(w[0]);
    r->mMessage = S32(w[1]);
    r->mCategory = S32(w[2]);
    r->mMode = S32(w[3]);
    r->mScorer = nullptr;   // the static initialiser of the scorers fills it
    r->mMaxDuplicates = S32(w[7]);
    r->mFirstResponse = S32(w[8]);
    r->mNumResponses = S32(w[9]);
    r->mCycling = S32(w[10]);
    r->mWord2c = S32(w[11]);
}

bool AdvisorMessages(const View& v, AdvisorMessageRecord* block, std::string* why) {
    uint32_t w[12];
    for (int i = 0; i < kGameTableMessageRecords; i++) {
        if (!ReadWords(v, kMessages + 0x30 * i, 12, w, why)) return false;
        if (w[4] != 0 || w[5] != 0 || w[6] != 0)
            return Fail(why, "the scorer slot is not empty (a static initialiser fills it at run time)");
        SetRecord(&block[1 + i], w);
    }
    // record -1: the 0x30 bytes before the table, the scorer table's last four pointers to member
    if (!ReadWords(v, kMessages - 0x30, 12, w, why)) return false;
    SetRecord(&block[0], w);
    std::memset(static_cast<void*>(&block[kGameTableMessageRecords + 1]), 0, sizeof(AdvisorMessageRecord));   // the spare
    return true;
}

bool AdvisorBalance(const View& v, BalanceDescription* rows, std::string* why) {
    static_assert(sizeof(BalanceDescription) == 0x3C, "BalanceDescription is 0x3c bytes");
    uint32_t a = 0x1028318C;
    for (int count = 0;; count++) {
        if (count == kGameTableBalanceRows) return Fail(why, "no type 12 row ends the balance description table");
        uint8_t raw[0x3C];
        if (!v.Read(a, 0x3C, raw, why)) return false;
        BalanceDescription& row = rows[count];
        row.mType = S32(U32(raw));
        std::memset(row.mName, 0, sizeof row.mName);
        for (int i = 0; i < 32 && raw[4 + i]; i++) {
            if (raw[4 + i] >= 0x80) return Fail(why, "a balance description name is not ASCII");
            row.mName[i] = char(raw[4 + i]);
        }
        row.mMinInteger = S32(U32(raw + 36));
        row.mMaxInteger = S32(U32(raw + 40));
        float bounds[2];
        for (int i = 0; i < 2; i++) {
            const uint32_t bits = U32(raw + 44 + 4 * i);
            std::memcpy(&bounds[i], &bits, 4);
        }
        if (!(bounds[0] == 0.0f && bounds[1] == 0.0f)) return Fail(why, "a balance description row has float bounds");
        row.mMinFloat = 0.0f;
        row.mMaxFloat = 0.0f;
        row.mArrayCount = S32(U32(raw + 52));
        row.mFlags = U32(raw + 56);
        a += 0x3C;
        if (row.mType == 12) {
            if (count + 1 != kGameTableBalanceRows)
                return Fail(why, "the balance description table has " + std::to_string(count + 1) + " rows, not " +
                                     std::to_string(kGameTableBalanceRows));
            return true;
        }
    }
}

bool LegalTexts(const View& v, std::u16string* texts, std::string* why) {
    uint32_t a = 0x10249F40;
    for (int i = 0; i < kGameTableLegalTexts; i++) {
        texts[i].clear();
        for (;;) {
            uint8_t b[2];
            if (!v.Read(a, 2, b, why)) return false;
            a += 2;
            const char16_t unit = char16_t(U16(b));
            if (unit == 0) break;
            texts[i] += unit;
        }
    }
    return true;
}

bool Scorers(const View& v, AdvisorScorer* scorers, std::string* why) {
    for (int k = 0; k < kGameTableMessageRecords; k++) {
        uint32_t w[3], code;
        if (!ReadWords(v, kScorerTable + 12 * k, 3, w, why)) return false;
        if (w[0] != 0 || w[1] != 0xFFFFFFFF) return Fail(why, "scorer " + std::to_string(k) + " is not a plain pointer to member");
        if (!v.Word(w[2], &code, why)) return false;
        const ScorerInfo* end = kScorers + kGameTableScorers;
        const ScorerInfo* it = std::lower_bound(kScorers, end, code, [](const ScorerInfo& s, uint32_t c) { return s.address < c; });
        if (it == end || it->address != code) return Fail(why, "scorer " + std::to_string(k) + " at " + Hex(code) + " is not declared");
        scorers[k] = it->scorer;
    }
    return true;
}

// The window script's words, as far as InterfaceWindow::Build reads them (gen_game_tables.py's window_script_words).
class ScriptWalk {
public:
    ScriptWalk(const View& v, uint32_t address, std::string* why) : mV(v), mAddress(address), mWhy(why) {}

    bool Count(uint32_t* words) {
        Level(-1);
        *words = mPos;
        return mOk;
    }

private:
    const View& mV;
    uint32_t mAddress;
    std::string* mWhy;
    uint32_t mPos = 0;
    bool mOk = true;

    int Word() {
        uint8_t b[2] = {0, 0};
        if (mOk && !mV.Read(mAddress + 2 * mPos, 2, b, mWhy)) mOk = false;
        mPos++;
        return int16_t(U16(b));
    }
    void Words(int n) {
        for (int i = 0; i < n && mOk; i++) Word();
    }

    void Level(int kind) {
        while (mOk) {
            const int r = Word();
            if (!mOk || r < 0 || r > 0x12 || r == 5) return;
            int child = -2;   // none
            if (r == 0) {
                child = Word();
                Words(8);                                   // attributes, id, rectangle
            } else if (r == 1 || r == 2 || r == 17 || r == 18 || r == 13) {
                Words(2);
            } else if (r == 3) {
                Words(4);
            } else if (r == 4) {
                const int shape = Word();
                if (shape == 4) Words(2 * Word());
                else Words(shape == 1 ? 4 : shape == 2 ? 3 : shape == 3 ? 4 : 0);
            } else if (r == 6 || r == 7 || r == 8 || r == 14 || r == 15) {
                Words(4);
                child = 2;
            } else if (r == 9 || r == 10) {
                if (kind == 4 || kind == 7) {               // a list box or a list control; elsewhere the record has no words
                    Words(4);
                    if (r == 9) child = 3;
                }
            } else if (r == 11) {
                Words(2 * Word());
            } else if (r == 12) {
                Words(5);
                child = 2;
            } else if (r == 16) {
                Words(5);
                child = 6;
            }
            if (child != -2) Level(child);
            if (mOk && mPos >= 4096) {
                mOk = false;
                Fail(mWhy, "the window script at " + Hex(mAddress) + " does not end");
            }
        }
    }
};

bool WindowScripts(const View& v, int16_t* const* scripts, std::string* why) {
    for (int i = 0; i < kGameTableWindowScripts; i++) {
        const ScriptInfo& s = kScripts[i];
        uint32_t words = 0;
        if (!ScriptWalk(v, s.address, why).Count(&words)) return false;
        if (words != uint32_t(s.words))
            return Fail(why, std::string("the window script ") + s.array + " at " + Hex(s.address) + " has " +
                                 std::to_string(words) + " words, not " + std::to_string(s.words));
        std::vector<uint8_t> b(2 * size_t(words));
        if (!v.Read(s.address, 2 * words, b.data(), why)) return false;
        for (uint32_t j = 0; j < words; j++) scripts[i][j] = int16_t(U16(b.data() + 2 * j));
    }
    return true;
}

bool RideTemplates(const View& v, uint32_t* words, std::string* why) {
    return ReadWords(v, 0x1029BE00, kGameTableRideTemplateWords, words, why);
}

bool ReadFile(const char* path, std::vector<uint8_t>* out, std::string* why) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return Fail(why, std::string("cannot open ") + path);
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size <= 0 || size > (std::streamoff(64) << 20)) return Fail(why, std::string(path) + " is not a program's data fork (its size)");
    in.seekg(0);
    out->resize(size_t(size));
    if (!in.read(reinterpret_cast<char*>(out->data()), size)) return Fail(why, std::string("cannot read ") + path);
    return true;
}

std::string& SourceName() {
    static std::string name;
    return name;
}

std::u16string* LoadedLegalTexts() {
    static std::u16string texts[kGameTableLegalTexts];
    return texts;
}

}  // namespace

int GameTableWindowScriptWords(int index) {
    return index >= 0 && index < kGameTableWindowScripts ? kScripts[index].words : 0;
}

const char* GameTableWindowScriptName(int index) {
    return index >= 0 && index < kGameTableWindowScripts ? kScripts[index].array : "";
}

const std::u16string& GameTableLegalText(int language) {
    static const std::u16string none;
    return language >= 0 && language < kGameTableLegalTexts ? LoadedLegalTexts()[language] : none;
}

const std::string& GameTableSource() { return SourceName(); }

bool ReadGameTables(const char* programDataFork, const GameTableDestinations& to, bool searchOnly, std::string* build,
                    std::string* why) {
    std::vector<uint8_t> file;
    if (!ReadFile(programDataFork, &file, why)) return false;
    const Sha1Digest id = Sha1(file.data(), file.size());
    const BuildInfo* known = nullptr;
    for (const BuildInfo& b : kBuilds)
        if (!searchOnly && b.size == file.size() && Sha1FromHex(b.sha1) == id) known = &b;
    Program program;
    if (!ParseProgram({file.data(), file.size()}, &program, why)) return false;
    file = std::vector<uint8_t>();
    View view(program);
    if (!PlaceBlocks(program, known, &view, why) || !FollowScorers(program, &view, why)) return false;
    std::memset(static_cast<void*>(to.lines), 0, sizeof(AdvisorLine) * kGameTableAdvisorLines);
    std::memset(static_cast<void*>(to.messageBlock), 0, sizeof(AdvisorMessageRecord) * (kGameTableMessageRecords + 2));
    std::memset(static_cast<void*>(to.balance), 0, sizeof(BalanceDescription) * kGameTableBalanceRows);
    if (!AdvisorLines(view, to.lines, why) || !AdvisorMessages(view, to.messageBlock, why) ||
        !AdvisorBalance(view, to.balance, why) || !Scorers(view, to.scorers, why) || !LegalTexts(view, to.legalTexts, why) ||
        !WindowScripts(view, to.windowScripts, why) || !RideTemplates(view, to.rideTemplates, why))
        return false;
    if (build) *build = known ? known->name : "found by content";
    return true;
}

}  // namespace tpw

#if TPW_TABLES_AT_CONFIGURE

// The tables are compiled in (devtools/gen_game_tables.py at configure time): nothing to read.
bool LoadGameTables(const char*, std::string*) {
    tpw::SourceName() = "compiled in";
    return true;
}

#else

// The tables' storage, filled by LoadGameTables (the configure-time build defines them in its generated sources instead).
AdvisorLine gAdvisorLines[611];
AdvisorMessageRecord gAdvisorMessageBlock[kAdvisorMessageCount + 2];
BalanceDescription gAdvisorBalanceDescriptions[775];
AdvisorScorer gAdvisorScorerSources[kAdvisorMessageCount];

// The window scripts' arrays, defined in src/game/ui with their sizes (a tables-off build's WindowScripts.inc fills them
// with zeros).
#define TPW_GAME_TABLE_WINDOW_SCRIPT(address, words, array, anchor, whole, ordinal, copies) extern int16_t array[];
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_WINDOW_SCRIPT

bool LoadGameTables(const char* programDataFork, std::string* why) {
    using namespace tpw;
    static_assert(sizeof(gAdvisorLines) / sizeof(gAdvisorLines[0]) == kGameTableAdvisorLines, "");
    static_assert(kAdvisorMessageCount == kGameTableMessageRecords, "");
    static_assert(sizeof(gAdvisorBalanceDescriptions) / sizeof(gAdvisorBalanceDescriptions[0]) == kGameTableBalanceRows, "");
    static_assert(sizeof(gSaveLegalText) / sizeof(gSaveLegalText[0]) == kGameTableLegalTexts, "");
    static_assert(sizeof(gRideTemplates) / sizeof(gRideTemplates[0]) == kGameTableRideTemplateWords, "");
    // Read into copies first, so a failure leaves the game's arrays as they were.
    std::vector<AdvisorLine> lines(kGameTableAdvisorLines);
    std::vector<AdvisorMessageRecord> messages(kGameTableMessageRecords + 2);
    std::vector<BalanceDescription> balance(kGameTableBalanceRows);
    std::vector<AdvisorScorer> scorers(kGameTableMessageRecords);
    std::vector<std::u16string> legal(kGameTableLegalTexts);
    std::vector<uint32_t> rides(kGameTableRideTemplateWords);
    std::vector<std::vector<int16_t>> scripts(kGameTableWindowScripts);
    GameTableDestinations to = {lines.data(), messages.data(), balance.data(), scorers.data(), legal.data(), {}, rides.data()};
    for (int i = 0; i < kGameTableWindowScripts; i++) {
        scripts[i].resize(size_t(GameTableWindowScriptWords(i)));
        to.windowScripts[i] = scripts[i].data();
    }
    std::string build;
    if (!ReadGameTables(programDataFork, to, false, &build, why)) return false;
    std::memcpy(static_cast<void*>(gAdvisorLines), lines.data(), sizeof gAdvisorLines);
    std::memcpy(static_cast<void*>(gAdvisorMessageBlock), messages.data(), sizeof gAdvisorMessageBlock);
    std::memcpy(static_cast<void*>(gAdvisorBalanceDescriptions), balance.data(), sizeof gAdvisorBalanceDescriptions);
    std::copy(scorers.begin(), scorers.end(), gAdvisorScorerSources);
    std::memcpy(gRideTemplates, rides.data(), sizeof gRideTemplates);
    int16_t* const arrays[] = {
#define TPW_GAME_TABLE_WINDOW_SCRIPT(address, words, array, anchor, whole, ordinal, copies) array,
#include "platform/game_table_lists.inc"
#undef TPW_GAME_TABLE_WINDOW_SCRIPT
    };
    for (int i = 0; i < kGameTableWindowScripts; i++)
        std::memcpy(arrays[i], scripts[i].data(), sizeof(int16_t) * scripts[i].size());
    for (int i = 0; i < kGameTableLegalTexts; i++) {
        LoadedLegalTexts()[i] = legal[i];
        gSaveLegalText[i] = LoadedLegalTexts()[i].c_str();
    }
    SourceName() = build;
    return true;
}

#endif
