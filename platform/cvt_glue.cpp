// cvt_glue.cpp: the Metrowerks double-to-unsigned conversion glue at 0x10237cd8, which three
// game headers name by its address (FloatToCell in include/TPCoaster.h, FLY_FloatToInt in
// include/HLSprite.h, FloatToIntGlue in src/RideTypes_CalculateRootNodeFaceNormals.cpp).
// The routine compares against its TOC constants [0.0, 4294967296.0, 2147483648.0]: below
// zero it gives 0, at 2^32 or above (or NaN, an unordered compare) 0xFFFFFFFF, below 2^31 a
// truncating fctiwz, and otherwise the truncation of x - 2^31 with the top bit set.
// Nothing here is a translation of game code.
#include <cstdint>

#include "HLSprite.h"
#include "TPCoaster.h"

int32_t FloatToIntGlue(float value);

namespace {
int32_t CvtDoubleToUnsigned(double x) {
    if (x < 0.0) return 0;
    if (!(x < 4294967296.0)) return static_cast<int32_t>(0xffffffffu);
    if (x < 2147483648.0) return static_cast<int32_t>(x);
    return static_cast<int32_t>(static_cast<uint32_t>(static_cast<int32_t>(x - 2147483648.0)) + 0x80000000u);
}
}  // namespace

int32_t FloatToCell(double value) { return CvtDoubleToUnsigned(value); }
int32_t FloatToIntGlue(float value) { return CvtDoubleToUnsigned(value); }
int32_t FLY_FloatToInt(float value) { return CvtDoubleToUnsigned(value); }
