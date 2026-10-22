// samsutils_version.cpp: Sam's Utilities' version object and string assignment, for the game code that prints the game's version
// (UI_CreateVersionNumber). The Mac library reads the application's 'vers' resource (UVersion::LoadFromResource, 0x10018cc8); this
// build has no resource fork, so the default version resource answers the game's own version.
//
// Where the version comes from. The 'vers' resources of the game's data disc (extracted/disc-hfs/Theme Park Data/SimTheme Park.rsrc:
// number 1 is "2.0.1", number 2 "SimTheme Park 2.0.1") belong to the release build, and the 2.04b3 patch ships the program's data
// fork only, so the text of the patched program's resource is not available: it is taken to be "2.04b3", the name of the patch the
// translated code comes from.
#include <cstdlib>
#include <cstring>

#include "platform/samsutils.h"

namespace SamsUtilities {

// 0x10008044. The text is copied (the library frees the old text first, then allocates and copies; an empty source leaves the shared
// empty text). Assigning a string to itself is harmless here (the library's would free the text it then copies).
UString& UString::operator=(const UString& other) {
    if (this == &other) return *this;
    const char* source = other.mText;
    const uint32_t length = other.mLength;
    if (mText != nullptr && mLength != 0) std::free(mText);
    if (length != 0 && source != nullptr && source[0] != '\0') {
        char* copy = static_cast<char*>(std::malloc(length + 1));
        std::memcpy(copy, source, length);
        copy[length] = '\0';
        mText = copy;
        mLength = length;
    } else {
        mText = const_cast<char*>(kEmptyString.mText);
        mLength = 0;
    }
    return *this;
}

// 0x100190b4 (__sinit_UVersion_cp): the 'vers' resource number 1.
const UResourceId UVersion::kDefaultVersionResource(UString("vers"), 1);

// 0x100188d8: the numbers as given, the stage 0x80 and no text (the library then formats the text from the numbers in AdjustText;
// the host keeps it empty until a resource is loaded).
UVersion::UVersion(uint16_t major, uint8_t minor, uint8_t fix, uint16_t build)
    : mMajor(major), mMinor(minor), mFix(fix), mStage(0x80), mBuild(build), mRegion(0), mText(), mComment() {}

// 0x10018cc8. The default resource ('vers' 1) is the game's own version; any other is not found, which leaves the object as it is.
void UVersion::LoadFromResource(const UResourceId& resource, uint8_t) {
    if (resource.mId != kDefaultVersionResource.mId || std::strcmp(resource.mType.mText, kDefaultVersionResource.mType.mText) != 0) return;
    mMajor = 2;
    mText = UString("2.04b3");
    mComment = UString();
}

}  // namespace SamsUtilities
