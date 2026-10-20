// dialogs.h: what the platform shows in place of the Windows message box (platform/dialogs.cpp).
#pragma once
#include <cstdint>
#include <string>

namespace tpw {

// UTF-16 to UTF-8 (surrogate pairs combined; a lone surrogate becomes U+FFFD). NULL gives "(null)".
std::string Utf8(const char16_t* text);

// The log line NS_MacDoze::MessageBoxW writes (without the "platform: " prefix).
std::string MessageBoxLine(const char16_t* text, const char16_t* title, uint32_t flags);

}  // namespace tpw
