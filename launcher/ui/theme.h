// theme.h: the launcher screen's name, wording and look. The look is a generic fairground scheme: orange and
// sunshine-yellow panels, lime "go" buttons, a sky-blue backdrop, thick dark outlines, rounded corners and soft shadows.
// Every colour below was picked by eye for this file. None is sampled from any game image, and the screen draws no
// logo, character, ride or wordmark of the game (launcher-design section 5 and section 8).
#pragma once
#include <cstdint>

namespace tpw::ui {

// The product's name as the player reads it (window title, home screen wordmark, About, --help), in one place so it can
// be renamed. The per-user data folder cannot hold a slash, so it has its own name just below.
inline constexpr const char* kProductName = "STP/TPW Launcher";
// The product name for the per-user data folder (SDL_GetPrefPath's application name; on macOS
// ~/Library/Application Support/STP-TPW Launcher). The same words without the slash.
inline constexpr const char* kDataFolderName = "STP-TPW Launcher";
// How the games are mentioned: descriptively, never as a logo (launcher-design section 5, "Trademark naming").
inline constexpr const char* kTagline =
    "An unofficial port for playing your own copy of Theme Park World (PC) and SimTheme Park (Mac).";
// Shown on the first screen and in the About box, word for word.
inline constexpr const char* kDisclaimer =
    "Not affiliated with or endorsed by Electronic Arts. Theme Park World and SimTheme Park are trademarks of their "
    "respective owners. No game content is included; you need your own discs.";

struct Rgba {
    uint8_t r, g, b, a;
    constexpr Rgba WithAlpha(uint8_t alpha) const { return {r, g, b, alpha}; }
};

namespace colour {
inline constexpr Rgba kSkyTop{86, 178, 238, 255};
inline constexpr Rgba kSkyBottom{178, 226, 252, 255};
inline constexpr Rgba kCloud{255, 255, 255, 170};
inline constexpr Rgba kOrange{255, 148, 46, 255};
inline constexpr Rgba kOrangeDeep{226, 104, 24, 255};
inline constexpr Rgba kYellow{255, 211, 66, 255};
inline constexpr Rgba kYellowSoft{255, 236, 160, 255};
inline constexpr Rgba kLime{136, 206, 44, 255};
inline constexpr Rgba kLimeBright{162, 228, 70, 255};
inline constexpr Rgba kLimeDeep{88, 152, 22, 255};
inline constexpr Rgba kCream{255, 250, 238, 255};
inline constexpr Rgba kPaper{255, 255, 255, 255};
inline constexpr Rgba kInk{42, 30, 62, 255};        // outlines and text
inline constexpr Rgba kInkSoft{96, 82, 112, 255};   // secondary text
inline constexpr Rgba kRed{234, 80, 76, 255};
inline constexpr Rgba kSkyChip{120, 196, 242, 255};
inline constexpr Rgba kGrey{196, 194, 204, 255};
inline constexpr Rgba kShadow{28, 18, 48, 80};
inline constexpr Rgba kDim{28, 18, 48, 140};         // behind the About box
}  // namespace colour

namespace metric {
inline constexpr float kMargin = 16;     // the window's side margins, in points
inline constexpr float kGap = 12;        // between panels
inline constexpr float kRadius = 16;     // panel corners
inline constexpr float kOutline = 3;     // the dark outline of panels, tiles and buttons
inline constexpr float kShadowDrop = 5;  // how far a shadow sits below its panel
inline constexpr float kShadowBlur = 10;
inline constexpr int kMinWidth = 720;    // the smallest window the layout is made for
inline constexpr int kMinHeight = 520;
inline constexpr int kStartWidth = 800;
inline constexpr int kStartHeight = 560;
}  // namespace metric

}  // namespace tpw::ui
