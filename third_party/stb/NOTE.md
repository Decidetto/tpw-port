# stb (stb_truetype.h, stb_rect_pack.h)

The launcher's screen (launcher/ui/) draws its text with stb_truetype: the fonts in third_party/fonts are rasterised
into glyph atlases with stbtt_PackFontRanges, which packs with stb_rect_pack. Only these two headers are kept.

- Project: stb, single-file public domain libraries for C/C++, by Sean Barrett and contributors.
- Origin: https://github.com/nothings/stb, commit a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d (master on 2026-10-19).
- Versions: stb_truetype.h v1.26, stb_rect_pack.h v1.01 (the version lines at the top of each header).
- License: public domain (Unlicense) or MIT, at the user's choice: LICENSE here, and the same text at the end of each
  header.
- Downloaded 2026-10-22 from https://raw.githubusercontent.com/nothings/stb/<commit>/<file>.

| File | Bytes | SHA-256 | git blob |
|---|---|---|---|
| stb_truetype.h | 199192 | ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab | 90a5c2e2b3fe563c98585294ca7a949876aec94c |
| stb_rect_pack.h | 20199 | 6388366a30de57c1b4c29b6eed4ed1c7d758fc1443d2dd95cfce943370bb3aa1 | 6a633ce666a8600d7d974996fea072faabe939b3 |
| LICENSE | 2510 | bebfe904b14301657e4e5d655c811d51fd31b97c455b9cc2d8600d6bac6cff63 | a77ae91f3ec0abb0028c9d9bd8d34d880b454d38 |

The project publishes no checksums. The SHA-256 values are of our download. Each file's git blob id was checked against
the one GitHub's tree listing gives for that commit, and all three match.

No local changes. launcher/ui/text.cpp is the one file that defines STB_TRUETYPE_IMPLEMENTATION and
STB_RECT_PACK_IMPLEMENTATION.
