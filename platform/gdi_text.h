// gdi_text.h: the CoreText side of gdi.cpp, in its own file because Apple's headers (MacTypes.h)
// define Rect and Point, which the game's headers define as well. Only standard types cross.
#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace tpw {
namespace gdi_text {

// Registers the font file for this process and remembers its faces (family, full and PostScript
// name); answers how many faces it has, 0 when it cannot be read.
int32_t AddFontFile(const std::string& hostPath);

// A message printed once per key to the platform log.
void LogOnce(const std::string& key, const std::string& message);

// What a LOGFONTA says.
struct Style {
    std::string face;
    int32_t height = 0;      // lfHeight (negative or positive: the size is |height|, 0 meaning 12)
    int32_t weight = 400;
    bool italic = false;
    bool underline = false;
};

// One face at one size.
class Font {
public:
    // The face of a registered font file, else a system font of that name, else Helvetica.
    static std::unique_ptr<Font> Create(const Style& style);
    ~Font();
    // QuickDraw's GetFontInfo ascent (rounded up) and TextWidth of the 92-character reference
    // string divided by 92 (truncated).
    int32_t Ascent() const;
    int32_t AverageWidth() const;
    // The family name of the face actually in use (Helvetica after a fallback); for the tests and logs.
    std::string FamilyName() const;
    // The width of the text in pixels at the size, unscaled; no kerning or ligatures.
    double Advance(const char* text, int32_t length) const;
    // Sets to 0xff every pixel of ink (width * height bytes, top row first) the glyphs cover,
    // without antialiasing: the text starts at column x on the baseline row, scaled
    // horizontally by scale from x. Pixels outside the image are clipped.
    void Draw(uint8_t* ink, int32_t width, int32_t height, int32_t x, int32_t baseline, double scale, const char* text,
              int32_t length) const;

private:
    Font();
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace gdi_text
}  // namespace tpw
