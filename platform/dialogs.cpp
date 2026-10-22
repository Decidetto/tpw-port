// dialogs.cpp: macdoze's MessageBoxW. The launcher runs unattended, so no box is shown: the
// title and the text go to the platform log (the error the game exits with, from
// CExitError::ReportAnyError, must stay visible on stderr) and the answer is IDOK.
#include "dialogs.h"

#include <cstdio>

#include "platform.h"
#include "platform/ltms.h"
#include "WinScrn.h"

namespace tpw {

std::string Utf8(const char16_t* text) {
    if (text == nullptr) return "(null)";
    std::string out;
    for (const char16_t* p = text; *p != 0; ++p) {
        uint32_t code = *p;
        if (code >= 0xd800 && code < 0xdc00 && p[1] >= 0xdc00 && p[1] < 0xe000) {
            code = 0x10000 + ((code - 0xd800) << 10) + (p[1] - 0xdc00);
            ++p;
        } else if (code >= 0xd800 && code < 0xe000) {
            code = 0xfffd;
        }
        if (code < 0x80) {
            out += static_cast<char>(code);
        } else if (code < 0x800) {
            out += static_cast<char>(0xc0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3f));
        } else if (code < 0x10000) {
            out += static_cast<char>(0xe0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
            out += static_cast<char>(0x80 | (code & 0x3f));
        } else {
            out += static_cast<char>(0xf0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
            out += static_cast<char>(0x80 | (code & 0x3f));
        }
    }
    return out;
}

std::string MessageBoxLine(const char16_t* text, const char16_t* title, uint32_t flags) {
    char tail[32];
    std::snprintf(tail, sizeof tail, " (flags 0x%x)", flags);
    return "message box \"" + Utf8(title) + "\": " + Utf8(text) + tail;
}

}  // namespace tpw

namespace NS_MacDoze {

int32_t MessageBoxW(GrafPort*, const char16_t* text, const char16_t* title, uint32_t flags) {
    tpw::Log("%s", tpw::MessageBoxLine(text, title, flags).c_str());
    return 1;   // IDOK
}

// macdoze 0x1000687c, MessageBoxA: the Mac shows a UDialog and answers the button pressed; here the title and the text go
// to the log, like MessageBoxW's, and the answer is IDOK.
int32_t MessageBoxA(GrafPort*, const char* text, const char* caption, uint32_t flags) {
    tpw::Log("message box \"%s\": %s (flags 0x%x)", caption != nullptr ? caption : "(null)", text != nullptr ? text : "(null)", flags);
    return 1;   // IDOK
}

}  // namespace NS_MacDoze
