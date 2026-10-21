// InterfaceWindow (main app, UI toolkit 0x101d98d8..0x101dc3e8): the constructor, PointIn and
// MakeRectAbsolute. The rest of the class is untranslated (include/InterfaceWindow.h lists
// its virtual members and data); later units add to this file.
#include "InterfaceWindow.h"

#include <cstring>
// TPW_PORT_FIX v5L26 InterfaceWindow | Mac: windows sit where the 2048-unit layout puts them | Port: on a wide screen a window placed on a wide one (the root, the game's view) is anchored: one in the left third of the layout stays at the left edge, one in the right third moves to the right edge, the rest (and full-screen art) move to the middle; the windows inside follow | Why: a widescreen port, with the panels at the screen's edges and the 4:3 screens pillarboxed rather than stretched
#include <unordered_map>

InterfaceWindow* gUILastWindow;   // 0x10395c18, the window the last constructor made

namespace {
// The shift each window placed straight on a wide window was given; looked up only for such windows, whose entry the
// constructor always writes, so an entry left by a window that has gone is never read.
std::unordered_map<const InterfaceWindow*, int16_t> gWideShifts;

// The windows above `window` that are not wide: each one's rectangle carries the shift once.
int32_t ShiftedAbove(const InterfaceWindow* window) {
    int32_t count = 0;
    for (const InterfaceWindow* parent = window->mParent; parent != nullptr && !UI_IsWideWindow(parent); parent = parent->mParent)
        ++count;
    return count;
}
}  // namespace

bool UI_IsWideWindow(const InterfaceWindow* window) {
    return window != nullptr && gUIUnitWidth > 0x800 && window->mRect.mRight - window->mRect.mLeft >= 0x800;
}

int16_t UI_WideShift(const InterfaceWindow* window) {
    if (gUIUnitWidth <= 0x800) return 0;
    for (const InterfaceWindow* w = window; w != nullptr && w->mParent != nullptr; w = w->mParent) {
        if (UI_IsWideWindow(w->mParent)) {
            const auto found = gWideShifts.find(w);
            return found == gWideShifts.end() ? 0 : found->second;
        }
    }
    return 0;
}

// The amount to add to a rectangle of the 2048-unit layout given in `window`'s frame: the anchor's shift, less the shifts
// MakeRectAbsolute adds again for a relative one (each window above that is not wide already carries it).
int16_t UI_WideShiftFor(const InterfaceWindow* window, bool relative) {
    const int32_t shift = UI_WideShift(window);
    if (shift == 0 || !relative) return static_cast<int16_t>(shift);
    return static_cast<int16_t>(shift - ShiftedAbove(window) * shift);
}

// The window's state is the Mac's field by field. The members that are not set here
// (0x68..0x87, 0xec, 0xf0 and 0x100..0x10f) are left as the allocation had them. The
// constructor reaches the base class's CalcClientArea, as the Mac's does while its vtable
// pointer is still the base's.
InterfaceWindow::InterfaceWindow(InterfaceWindow* parent, int32_t attributes, int32_t id, int16_t left, int16_t top,
                                 int16_t right, int16_t bottom) {
    mDestroying = 0;
    mAttributes = static_cast<uint32_t>(attributes);
    mCallback = UI_DefaultWindowCallback;
    mDefaultCallback = UI_DefaultWindowCallback;
    mParent = parent;
    mNext = nullptr;
    mChild = nullptr;
    mPrevious = nullptr;
    mOwner = nullptr;   // (the word at +0x60: zero on the Mac)
    mF64 = 0;
    mAnim = nullptr;
    mAnimating = 0;
    mAnimFrame = 0;
    mAnimOffset[3] = 0;
    mAnimOffset[2] = 0;
    mAnimOffset[1] = 0;
    mAnimOffset[0] = 0;
    mSlideTimer = nullptr;
    mHelpText = -1;
    mF90 = -1;
    for (int32_t i = 0; i < 4; ++i) mAnimTimers[i] = nullptr;
    mF134 = 0;
    mF130 = 0;
    mCursor = -1;
    mId = id;
    mButtonsHeld = 0;
    // (the anchoring: a window on a wide one is given its shift; one further in takes its anchor's)
    if (gUIUnitWidth > 0x800 && parent != nullptr) {
        int16_t shift = 0;
        if (UI_IsWideWindow(parent)) {
            if (right - left < 0x800) shift = UI_WideAnchor(left, right, gUIUnitWidth);
            gWideShifts[this] = shift;
        } else {
            shift = UI_WideShiftFor(this, (attributes & 0x400) == 0x400);
        }
        left = static_cast<int16_t>(left + shift);
        right = static_cast<int16_t>(right + shift);
    }
    mRect.mLeft = left;
    mRect.mTop = top;
    mRect.mRight = right;
    mRect.mBottom = bottom;
    if ((attributes & 0x400) == 0x400) MakeRectAbsolute(&mRect);
    CalcClientArea();
    gUILastWindow = this;
    mFont = nullptr;
    mDrawObject = nullptr;
    mClientDrawObject = nullptr;
    mRegion = &mRect;
    mOriginalRect.mLeft = mRect.mLeft;
    mOriginalRect.mTop = mRect.mTop;
    mOriginalRect.mRight = mRect.mRight;
    mOriginalRect.mBottom = mRect.mBottom;
    if (parent != nullptr) {
        mZ = static_cast<int16_t>(parent->mZ + 2);
        mPivotX = parent->mPivotX;
        mPivotY = parent->mPivotY;
    } else {
        mZ = 1;
        mPivotX = left;
        mPivotY = bottom;
    }
    // (the pivot is then the middle of the rectangle, whichever of the two the branch gave)
    mPivotX = static_cast<int16_t>(left + ((right - left) >> 1));
    mPivotY = static_cast<int16_t>(top + ((bottom - top) >> 1));
    if (mZ > 0x100) mZ = 0x100;
    mZRot = 0;
    std::memset(mExtraParams, 0, sizeof mExtraParams);
    if ((attributes & 1) != 0) UI_UpdatePointerIn();
    UI_PostMessage(this, 0x15, 0, 0);
}

// The deepest window under the point: a window that is neither disabled (attribute 2) nor
// being destroyed and is a kind PointIn answers for (attribute 1 or one of 0xf0000000)
// asks its children, in order, and the last that answers wins; then its own region says
// whether the point is in it (asked whether or not a child answered), which makes the
// window itself the answer only when no child did.
InterfaceWindow* InterfaceWindow::PointIn(int16_t x, int16_t y) {
    InterfaceWindow* found = nullptr;
    const uint32_t attributes = mAttributes;
    if ((attributes & 2) == 0 && mDestroying == 0 && ((attributes & 1) != 0 || (attributes & 0xf0000000) != 0)) {
        for (InterfaceWindow* child = mChild; child != nullptr; child = child->mNext) {
            InterfaceWindow* hit = child->PointIn(x, y);
            if (hit != nullptr) found = hit;
        }
        const bool inside = mRegion->PointIn(x, y);
        if (inside && found == nullptr) found = this;
    }
    return found;
}

// Shifts `rect` by the position of every window above this one (each parent's rectangle
// as it is now, so a parent that is itself absolute is counted with its own parents again).
void InterfaceWindow::MakeRectAbsolute(CoolRect* rect) {
    for (InterfaceWindow* parent = mParent; parent != nullptr; parent = parent->mParent)
        rect->Shift(parent->mRect.mLeft, parent->mRect.mTop);
}
