// versions.h: the About box's "Made with" table. Every version in it is read from the library or font itself (the
// headers of the copy the build compiles, the library at run time, a font's own name table), never written here.
#pragma once

#include <string>
#include <vector>

namespace tpw::ui {

class Text;

struct MadeWith {
    std::string name, version, license;
};

// The rows in the order the About box shows them. FFmpeg is listed only in a build that plays the films (TPW_MOVIES).
std::vector<MadeWith> MadeWithRows(const Text& text);

}  // namespace tpw::ui
