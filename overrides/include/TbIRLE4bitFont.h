// ltms: the 4-bit run-length font (the only font format the game ships). Mac size 0x18,
// vtable 0x10011334; the font file follows the object in its allocation.
//
// File (little-endian; swapped in place by the constructor on the Mac): u32 "F4FB", a
// byte, the line height (byte 5), u16 character count, then one u32 per character: the
// offset from the start of the file of its record, a 0x18-byte header followed by the
// run-length data. The header's width is the u16 at 0x16.
#pragma once
#include "TbResource.h"

class TbIResourceIndex;

// TPW_PORT_FIX v5L34 gTbFontScale | Mac: a glyph is drawn at its bitmap's size, so on a screen larger than the 1024 x 768 the largest fonts were made for the text shrinks against the UI around it | Port: the line height, the advances and the glyphs (resampled by DrawChar) are scaled by gTbFontScale, the UI's pixels per unit over 1024 x 768's | Why: text keeps the size the layout gives it at every resolution
#include <algorithm>
#include <cmath>
#include <vector>
inline float gTbFontScale = 1.0f;   // set by UI_SetRealScreenRez; 1 draws the fonts as they are
// The scale for a font with line height `line`: gTbFontScale, but no more than keeps the scaled line height in its byte.
inline float TbFontScaleFor(uint32_t line) {
    if (gTbFontScale <= 1.0f || line == 0) return 1.0f;
    const float cap = 255.0f / static_cast<float>(line);
    return gTbFontScale < cap ? gTbFontScale : cap;
}
inline uint32_t TbFontScaled(uint32_t value, float scale) {
    return scale == 1.0f ? value : static_cast<uint32_t>(static_cast<float>(value) * scale + 0.5f);
}
// DrawChar's (src/ltms_fontrender_method.cpp): the glyph `pixels` (width x height nybbles, rows packed) resampled by the
// scale for a font of line height `line`: bilinear, then the edges steepened again (more for a one-colour glyph, whose
// nybbles are all 0 or 0xf, than for an antialiased one, whose grey steps are part of the design), so the scaled text
// stays sharp. Answers the scaled glyph (in `out`) through the arguments, its offsets scaled with it.
inline void TbFontScaleGlyph(uint8_t line, const uint8_t*& pixels, uint16_t& width, uint16_t& height, int32_t& offsetX, int32_t& offsetY,
                             std::vector<uint8_t>& out) {
    float scale = TbFontScaleFor(line);
    if (scale == 1.0f || width == 0 || height == 0) return;
    if (static_cast<float>(width) * scale > 255.0f) scale = 255.0f / static_cast<float>(width);   // the writers' row length is a byte
    const int32_t sourceWidth = width;
    const int32_t sourceHeight = height;
    const uint8_t* const source = pixels;
    auto nybble = [&](int32_t x, int32_t y) -> int32_t {
        if (x < 0 || y < 0 || x >= sourceWidth || y >= sourceHeight) return 0;
        const uint32_t i = static_cast<uint32_t>(y * sourceWidth + x);
        return (i & 1) != 0 ? source[i >> 1] & 0xf : source[i >> 1] >> 4;
    };
    bool oneColour = true;
    for (int32_t y = 0; y < sourceHeight && oneColour; ++y)
        for (int32_t x = 0; x < sourceWidth && oneColour; ++x) {
            const int32_t value = nybble(x, y);
            oneColour = value == 0 || value == 0xf;
        }
    const float gain = std::max(1.0f, oneColour ? 0.75f * scale : 1.0f + 0.3f * (scale - 1.0f));
    const int32_t scaledWidth = std::max(1, static_cast<int32_t>(static_cast<float>(sourceWidth) * scale + 0.5f));
    const int32_t scaledHeight = std::max(1, static_cast<int32_t>(static_cast<float>(sourceHeight) * scale + 0.5f));
    out.assign(static_cast<size_t>((scaledWidth * scaledHeight + 1) >> 1), 0);
    for (int32_t y = 0; y < scaledHeight; ++y) {
        const float sy = (static_cast<float>(y) + 0.5f) / scale - 0.5f;
        const int32_t y0 = static_cast<int32_t>(std::floor(sy));
        const float fy = sy - static_cast<float>(y0);
        for (int32_t x = 0; x < scaledWidth; ++x) {
            const float sx = (static_cast<float>(x) + 0.5f) / scale - 0.5f;
            const int32_t x0 = static_cast<int32_t>(std::floor(sx));
            const float fx = sx - static_cast<float>(x0);
            const float top = static_cast<float>(nybble(x0, y0)) * (1.0f - fx) + static_cast<float>(nybble(x0 + 1, y0)) * fx;
            const float bottom = static_cast<float>(nybble(x0, y0 + 1)) * (1.0f - fx) + static_cast<float>(nybble(x0 + 1, y0 + 1)) * fx;
            const float coverage = std::clamp(((top * (1.0f - fy) + bottom * fy) / 15.0f - 0.5f) * gain + 0.5f, 0.0f, 1.0f);
            const uint8_t value = static_cast<uint8_t>(coverage * 15.0f + 0.5f);
            const uint32_t i = static_cast<uint32_t>(y * scaledWidth + x);
            out[i >> 1] |= (i & 1) != 0 ? value : static_cast<uint8_t>(value << 4);
        }
    }
    pixels = out.data();
    width = static_cast<uint16_t>(scaledWidth);
    height = static_cast<uint16_t>(scaledHeight);
    offsetX = static_cast<int32_t>(std::lround(static_cast<float>(offsetX) * scale));
    offsetY = static_cast<int32_t>(std::lround(static_cast<float>(offsetY) * scale));
}

class TbIRLE4bitFont : public TbIFont {
public:
    TB_RESOURCE_IN_ARRAY_BLOCK
    TbIRLE4bitFont();                                              // 0x100040d8
    TbIRLE4bitFont(unsigned char* data, TbIResourceIndex* index);  // 0x100041e4
    ~TbIRLE4bitFont() override;                                    // 0x10004150
    uint32_t GetCharIndex(const unsigned char*& text) override;    // 0x1000442c
    uint16_t GetCharCount() override;                              // 0x100044b4
    TbFontCharSize GetCharSize(uint16_t index) override;           // 0x10004508
    uint8_t GetVerticalSpacing() override;                         // 0x10004590
    uint32_t GetCaps() override;                                   // 0x10004638
    uint32_t GetType() override;                                   // 0x100045ec
    TbIMBToUnicode* GetMBToUnicode() override;                     // 0x100048ec
    const unsigned char* GetCharData(uint16_t index) const;        // 0x10004684
    const unsigned char* GetCharHeader(uint16_t index) const;      // 0x100046f8

    unsigned char* mData;           // Mac +8: the file
    unsigned char* mHeader;         // Mac +0xc: the same address
    uint32_t* mOffsets;             // Mac +0x10: the file + 8
    TbIMBToUnicode* mMBToUnicode;   // Mac +0x14: the language's converter, held with a reference
};
