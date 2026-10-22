#!/usr/bin/env python3
"""Regenerate assets/manifest_data.inc, the asset manifest the launcher compiles in (hashes, sizes and names only).

usage: python3 -I devtools/make_manifest.py [--scratch DIR] [--out FILE] [--check] [--verify-program EXTRACTED]

What a game folder must hold, one record per file, found by CONTENT (size and SHA-256), never by the disc image's MD5:

  US  (editions us)    import_originals.py CHECKSUMS, the "Theme Park Data" folder of the Mac CD: data/ without the
                       four Finder "Icon" files, the 17 font resource forks (data/fonts/*.rsrc: the game reads the
                       TrueType fonts out of them, the data forks are empty) and the program's data fork, with BOTH
                       accepted builds (CD 2.0.1 and patch 2.04b3) as SHA-256 values of one record.
  EUR (editions eur)   import_eur.py EXPECTED, all of the European folder (PC disc files, Mac files, tuning files,
                       resource forks and Finder info files); the sizes come from tree-ee54d817.json and the content
                       source (pc or mac) from draft-manifests.json, both written by the launcher-design study.
  Gold (editions gold) gold-wads.sha256 (the 35 ride wads) and the ride configuration text, sizes from gold-cab/.

A file whose destination, hashes, group, flags and source are the same in two editions is ONE record with both
edition bits. Pressing 66100's differing files have no alternate hashes (decided 2026-10-21).

The scratch folder is the launcher-design study's (draft-manifests.json, tree-*.json, gold-wads.sha256, gold-cab/); no
disc is needed. --check compares instead of writing (exit 1 when the file would change). --verify-program EXTRACTED
(ROOT/extracted) recomputes the two program builds' SHA-1 and SHA-256 from the files and fails if they differ from the
constants below.

Python 3.8 or later and nothing else. The lists are read as text (ast), never imported.
"""
import argparse
import ast
import hashlib
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DECOMP = os.path.dirname(HERE)
DEFAULT_SCRATCH = "/Volumes/6TB-2024/_scratch/wiring/launcher-design"
DEFAULT_OUT = os.path.join(DECOMP, "assets", "manifest_data.inc")

# The two builds of the program the launcher accepts (SHA-1 as the study and gen_game_tables.py name them), and the
# SHA-256 the lists carry for each; --verify-program recomputes both from the files.
PROGRAM_CD = ("45d731f0d059a7dde6caa1142545fcd3a7fadd43",
              "04809cd4ccee5433c7fb0b7c93d32f6a7aa629c1849181c0b7906415e5e295f5", 2274758)
PROGRAM_PATCH = ("ce184baca37824def7faa1fb033840a2ce273ef9",
                 "29da2392beefc1ee00b097536b7f1b202044cbf0743c0eb3a6235254fd318000", 2746951)
PROGRAM_DEST = "SimTheme Park"

GOLD_DEST_FOLDER = "gold"
GOLD_CONFIG = "RideConfig.txt"
EXPECTED_WADS = 35

# the codes the header (assets/manifest.h) names
GROUPS = ["core", "fonts-rsrc", "movies", "language:american", "language:english", "program", "gold"]
GROUP_NAME = {"core": "kGroupCore", "fonts-rsrc": "kGroupFontsRsrc", "movies": "kGroupMovies",
              "language:american": "kGroupLanguageAmerican", "language:english": "kGroupLanguageEnglish",
              "program": "kGroupProgram", "gold": "kGroupGold"}
EDITION_BIT = {"us": 1, "eur": 2, "gold": 4}
EDITION_NAME = {"us": "kEditionUs", "eur": "kEditionEur", "gold": "kEditionGold"}
SOURCE_NAME = {"mac": "kSourceMacCd", "pc": "kSourcePcCd", "gold": "kSourceGoldCd"}
FLAG_REQUIRED, FLAG_RSRC, FLAG_FINFO, FLAG_TUNING = 1, 2, 4, 8
MAC_TUNING = ("data/med.sam", "data/sound.sam")


class Fail(Exception):
    pass


def fail(msg):
    raise Fail(msg)


# ---- the lists, read as text

def module_constants(path):
    """The string and number constants assigned at the top level of the Python file PATH (no code runs)."""
    with open(path, "r", encoding="utf-8") as f:
        tree = ast.parse(f.read(), path)
    out = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
            try:
                out[node.targets[0].id] = ast.literal_eval(node.value)
            except ValueError:
                pass
    return out


def checksums():
    """{relative path under the disc/patch folders: (size, sha256)} from import_originals.py CHECKSUMS."""
    c = module_constants(os.path.join(HERE, "import_originals.py"))
    out = {}
    folder = ""
    for line in c["CHECKSUMS"].split("\n"):
        if not line:
            continue
        if line.startswith("= "):
            folder = line[2:]
            continue
        sha, size, name = line.split(" ", 2)
        out[folder + name] = (int(size), sha)
    return out, c


def expected_eur():
    """{relative path: sha256} from import_eur.py EXPECTED (all 913 entries)."""
    c = module_constants(os.path.join(HERE, "import_eur.py"))
    out = {}
    for line in c["EXPECTED"].strip().splitlines():
        digest, rel = line.split("  ", 1)
        if rel in out:
            fail("EXPECTED lists %s twice" % rel)
        out[rel] = digest
    return out


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# ---- classification

def group_of(path):
    low = path.lower()
    if low.startswith("data/fonts/"):
        return "fonts-rsrc"
    if low.startswith("data/movies/"):
        return "movies"
    if low.startswith("data/language/american/"):
        return "language:american"
    if low.startswith("data/language/english/"):
        return "language:english"
    return "core"


def is_font_resource_fork(path):
    return path.startswith("data/fonts/") and path.endswith(".rsrc")


# ---- the sets. A candidate is (dest, [(size, sha256)], group, edition, source, flags)

def us_set(cs, c):
    base = c["DISC"] + "/" + c["GAME_FOLDER"] + "/"
    patch = c["PATCH"] + "/" + c["PATCH_FOLDER"] + "/patch 2.04b3/SimTheme Park"
    out = []
    for path, (size, sha) in sorted(cs.items()):
        if not path.startswith(base):
            continue
        rel = path[len(base):]
        if rel.endswith(".finfo") or (rel.endswith(".rsrc") and not is_font_resource_fork(rel)):
            continue
        base_name = rel.rsplit("/", 1)[-1]
        if base_name == "Icon\r":
            continue
        if rel == PROGRAM_DEST:
            continue                                   # below, with both builds
        if not rel.startswith("data/"):
            continue
        flags = FLAG_REQUIRED | (FLAG_RSRC if rel.endswith(".rsrc") else 0)
        out.append((rel, [(size, sha)], group_of(rel), "us", "mac", flags))
    cd = cs.get(base + PROGRAM_DEST)
    pt = cs.get(patch)
    if cd != (PROGRAM_CD[2], PROGRAM_CD[1]):
        fail("CHECKSUMS has no CD program with the expected size and SHA-256 (%r)" % (cd,))
    if pt != (PROGRAM_PATCH[2], PROGRAM_PATCH[1]):
        fail("CHECKSUMS has no patch program with the expected size and SHA-256 (%r)" % (pt,))
    out.append((PROGRAM_DEST, [cd, pt], "program", "us", "mac", FLAG_REQUIRED))
    return out


def eur_set(exp, tree, draft):
    src = {x["path"]: x["source"] for x in draft["eur"]}
    for x in draft["eur"]:
        if tree.get(x["path"]) != [x["size"], x["sha256"]]:
            fail("draft-manifests.json and tree-ee54d817.json disagree on %s" % x["path"])
    if set(exp) != set(tree):
        fail("EXPECTED and tree-ee54d817.json list different paths (%d, %d)" % (len(exp), len(tree)))
    out = []
    for path in sorted(exp):
        size, sha = tree[path]
        if sha != exp[path]:
            fail("EXPECTED and tree-ee54d817.json disagree on the hash of %s" % path)
        side = path.rsplit(".", 1)[-1] if path.endswith((".rsrc", ".finfo")) else ""
        owner = path[:-len("." + side)] if side else path
        if owner not in src:
            fail("%s has no content source in draft-manifests.json" % owner)
        source = "mac" if src[owner] in ("mac", "mac-tuning") else "pc"
        flags = 0
        if side == "rsrc":
            flags |= FLAG_RSRC
        if side == "finfo":
            flags |= FLAG_FINFO
        if owner in MAC_TUNING:
            flags |= FLAG_TUNING
        if side == "" or is_font_resource_fork(path):
            flags |= FLAG_REQUIRED
        out.append((path, [(size, sha)], group_of(path), "eur", source, flags))
    return out


def gold_set(scratch, shalist):
    folder = os.path.join(scratch, "gold-cab")
    out = []
    with open(shalist, "r", encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            sha, name = line.split("  ", 1)
            name = name.split("/", 1)[1]
            p = os.path.join(folder, name)
            if sha256_file(p) != sha:
                fail("gold-cab/%s does not match gold-wads.sha256" % name)
            out.append((GOLD_DEST_FOLDER + "/" + name, [(os.path.getsize(p), sha)], "gold", "gold", "gold",
                        FLAG_REQUIRED))
    if len(out) != EXPECTED_WADS:
        fail("gold-wads.sha256 lists %d wads, expected %d" % (len(out), EXPECTED_WADS))
    p = os.path.join(folder, GOLD_CONFIG)
    out.append((GOLD_DEST_FOLDER + "/" + GOLD_CONFIG, [(os.path.getsize(p), sha256_file(p))], "gold", "gold", "gold",
                FLAG_REQUIRED))
    return out


def merge(candidates):
    """One record per (dest, hashes, group, flags, source); the editions of equal candidates are or-ed together."""
    recs = {}
    for dest, hashes, group, edition, source, flags in candidates:
        key = (dest, tuple(hashes), group, flags, source)
        recs[key] = recs.get(key, 0) | EDITION_BIT[edition]
    out = []
    for (dest, hashes, group, flags, source), editions in recs.items():
        out.append((dest, list(hashes), group, editions, source, flags))
    out.sort(key=lambda r: (r[0].encode("utf-8"), r[3], r[4], r[5]))
    seen = set()
    for r in out:
        for e, bit in EDITION_BIT.items():
            if r[3] & bit:
                if (r[0], e) in seen:
                    fail("%s is listed twice for %s with different content" % (r[0], e))
                seen.add((r[0], e))
    return out


# ---- the C++ text

def c_string(s):
    out = ['"']
    for ch in s:
        o = ord(ch)
        if ch == '"' or ch == "\\":
            out.append("\\" + ch)
        elif 0x20 <= o < 0x7F and ch != "?":
            out.append(ch)
        elif o < 0x80:
            out.append("\\%03o" % o)
        else:
            fail("a destination with a non-ASCII character: %r" % s)
    out.append('"')
    return "".join(out)


def flag_text(flags):
    names = []
    for bit, name in ((FLAG_REQUIRED, "kFlagRequired"), (FLAG_RSRC, "kFlagRsrc"), (FLAG_FINFO, "kFlagFinfo"),
                      (FLAG_TUNING, "kFlagTuning")):
        if flags & bit:
            names.append(name)
    return "|".join(names) if names else "0"


def edition_text(bits):
    return "|".join(EDITION_NAME[e] for e in ("us", "eur", "gold") if bits & EDITION_BIT[e])


def render(records):
    hashes = []                       # (sha bytes, size, record index)
    for ri, (_, hs, _, _, _, _) in enumerate(records):
        for size, sha in hs:
            hashes.append((bytes.fromhex(sha), size, ri))
    first = []
    n = 0
    for _, hs, _, _, _, _ in records:
        first.append(n)
        n += len(hs)
    by_hash = sorted(range(len(hashes)), key=lambda i: (hashes[i][0], hashes[i][1], hashes[i][2]))

    counts = {}
    for e in ("us", "eur", "gold"):
        counts[e] = sum(1 for r in records if r[3] & EDITION_BIT[e])

    o = []
    o.append("// manifest_data.inc: generated by devtools/make_manifest.py; do not edit. Hashes, sizes and names only.\n")
    o.append("// Included once, by assets/manifest.cpp, inside namespace tpw::assets.\n")
    o.append("// %d records (us %d, eur %d, gold %d; a record shared by two editions is counted in both), %d hashes.\n"
             % (len(records), counts["us"], counts["eur"], counts["gold"], len(hashes)))
    o.append("\n")
    o.append("// One accepted content: SHA-256, size in bytes and the record it belongs to. A record's hashes are\n")
    o.append("// consecutive, from its first_hash.\n")
    o.append("const ManifestHash kManifestHashes[] = {\n")
    for sha, size, ri in hashes:
        o.append("    {{%s}, %du, %du},\n" % (",".join("0x%02x" % b for b in sha), size, ri)
                 if size < (1 << 32) else
                 "    {{%s}, %dull, %du},\n" % (",".join("0x%02x" % b for b in sha), size, ri))
    o.append("};\n\n")
    o.append("// dest, first_hash, hash_count, group, editions, source, flags; sorted by dest (bytes), then editions.\n")
    o.append("const ManifestRecord kManifestRecords[] = {\n")
    for ri, (dest, hs, group, editions, source, flags) in enumerate(records):
        o.append("    {%s, %du, %d, %s, %s, %s, %s},\n"
                 % (c_string(dest), first[ri], len(hs), GROUP_NAME[group], edition_text(editions), SOURCE_NAME[source],
                    flag_text(flags)))
    o.append("};\n\n")
    o.append("// Indexes into kManifestHashes, sorted by SHA-256, then size, then record: the lookup by content.\n")
    o.append("const uint32_t kManifestByHash[] = {\n")
    line = "   "
    for i in by_hash:
        item = " %du," % i
        if len(line) + len(item) > 118:
            o.append(line + "\n")
            line = "   "
        line += item
    o.append(line + "\n};\n")
    return "".join(o)


def verify_program(extracted):
    pairs = (("disc-hfs/Theme Park Data/SimTheme Park", PROGRAM_CD),
             ("SimTPPatch_204b3/Sim Theme Park Patch/patch 2.04b3/SimTheme Park", PROGRAM_PATCH))
    for rel, (sha1, sha256, size) in pairs:
        with open(os.path.join(extracted, rel), "rb") as f:
            data = f.read()
        if (hashlib.sha1(data).hexdigest(), hashlib.sha256(data).hexdigest(), len(data)) != (sha1, sha256, size):
            fail("%s does not match the program constants" % rel)
        print("program ok: %s %d bytes" % (sha1, size))


def build(scratch):
    cs, c = checksums()
    exp = expected_eur()
    tree_us = load_json(os.path.join(scratch, "tree-e921f604.json"))
    tree_eur = load_json(os.path.join(scratch, "tree-ee54d817.json"))
    draft = load_json(os.path.join(scratch, "draft-manifests.json"))
    # CHECKSUMS against the study's own hashes of the same Mac folder (nothing is taken from the study for the US set)
    base = c["DISC"] + "/" + c["GAME_FOLDER"] + "/"
    mine = {p[len(base):]: list(v) for p, v in cs.items() if p.startswith(base)}
    for p, v in mine.items():
        if tree_us.get(p) != v:
            fail("CHECKSUMS and tree-e921f604.json disagree on %s" % p)
    cand = us_set(cs, c) + eur_set(exp, tree_eur, draft) + gold_set(scratch, os.path.join(scratch, "gold-wads.sha256"))
    return merge(cand)


def summary(records):
    lines = []
    for e, bit in EDITION_BIT.items():
        rs = [r for r in records if r[3] & bit]
        by_group = {}
        for r in rs:
            by_group[r[2]] = by_group.get(r[2], 0) + 1
        lines.append("%-5s %4d records; groups %s" % (e, len(rs), ", ".join("%s %d" % (g, by_group[g]) for g in GROUPS
                                                                        if g in by_group)))
        req = sum(1 for r in rs if r[5] & FLAG_REQUIRED)
        lines.append("      required %d, optional %d, rsrc %d, finfo %d, tuning %d; sources %s"
                     % (req, len(rs) - req, sum(1 for r in rs if r[5] & FLAG_RSRC), sum(1 for r in rs if r[5] & FLAG_FINFO),
                        sum(1 for r in rs if r[5] & FLAG_TUNING),
                        ", ".join("%s %d" % (s, sum(1 for r in rs if r[4] == s)) for s in ("mac", "pc", "gold")
                                  if any(r[4] == s for r in rs))))
    return "\n".join(lines)


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--scratch", default=DEFAULT_SCRATCH, help="the launcher-design study's folder")
    ap.add_argument("--out", default=DEFAULT_OUT)
    ap.add_argument("--check", action="store_true", help="compare with --out instead of writing it")
    ap.add_argument("--verify-program", metavar="EXTRACTED", help="recompute the program builds' hashes from ROOT/extracted")
    a = ap.parse_args(argv)
    try:
        if a.verify_program:
            verify_program(a.verify_program)
        records = build(a.scratch)
        text = render(records)
    except Fail as e:
        sys.exit("make_manifest: %s" % e)
    print(summary(records))
    if a.check:
        with open(a.out, "r", encoding="utf-8", newline="") as f:
            same = f.read() == text
        print("up to date" if same else "DIFFERENT from %s" % a.out)
        return 0 if same else 1
    with open(a.out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s" % a.out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
