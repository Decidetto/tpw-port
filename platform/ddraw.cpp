// ddraw.cpp: the DirectDraw objects the engine's display start-up (tb_ddraw.cpp) creates.
// On the Mac these came from indirectx; under its OpenGL implementation the picture is
// drawn with OpenGL and DirectDraw only frames it: a display mode, a primary surface with
// a back buffer, a clipper. So the surfaces here are plain memory the engine can lock (it
// does for screenshots), and nothing is presented from them: the frame reaches the screen
// through UOpenGL::Swap.
//
// What is reported to the engine (pixel formats, capability words, video memory) is this
// layer's choice, not a reproduction of indirectx: see the notes on each call.
#include <SDL3/SDL.h>

#include <cstring>
#include <unordered_map>
#include <vector>

#include "platform.h"
#include "platform/indirectx.h"
#include "platform/macdoze.h"

namespace {
const int32_t DD_OK = 0;
const int32_t E_NOINTERFACE = static_cast<int32_t>(0x80004002);
const int32_t DDERR_INVALIDPARAMS = static_cast<int32_t>(0x80070057);
const int32_t DDERR_NOTFOUND = static_cast<int32_t>(0x887600ff);

// Reference counts of the COM-style objects (IUnknown has no room for one).
std::unordered_map<NS_MacDoze::IUnknown*, uint32_t>& RefCounts() {
    static auto& counts = *new std::unordered_map<NS_MacDoze::IUnknown*, uint32_t>;   // (never destroyed: see file_handles.cpp)
    return counts;
}
void RetainObject(NS_MacDoze::IUnknown* object) { ++RefCounts()[object]; }

// The display mode the game set, or the window's size before it sets one.
struct DisplayMode {
    uint32_t width = 640;
    uint32_t height = 480;
    uint32_t depth = 32;
};
DisplayMode gMode;

void FillPixelFormat(DDPIXELFORMAT& format, uint32_t depth) {
    std::memset(&format, 0, sizeof format);
    format.dwSize = sizeof format;
    format.dwFlags = DDPF_RGB;
    format.dwRGBBitCount = depth;
    if (depth == 16) {   // 5-6-5
        format.dwRBitMask = 0xf800;
        format.dwGBitMask = 0x07e0;
        format.dwBBitMask = 0x001f;
    } else {             // 8-8-8-8
        format.dwRBitMask = 0x00ff0000;
        format.dwGBitMask = 0x0000ff00;
        format.dwBBitMask = 0x000000ff;
        format.dwRGBAlphaBitMask = 0xff000000;
    }
}

class Surface : public IDirectDrawSurface4 {
public:
    Surface(uint32_t width, uint32_t height, uint32_t depth, uint32_t caps)
        : mWidth(width), mHeight(height), mDepth(depth), mCaps(caps),
          mPixels(static_cast<size_t>(width) * height * (depth / 8)) {}
    ~Surface() override {
        if (mBack != nullptr) mBack->Release();
    }
    void Describe(DDSURFACEDESC2* desc) const {
        std::memset(desc, 0, sizeof *desc);
        desc->dwSize = sizeof *desc;
        desc->dwFlags = 0x100f;   // caps, height, width, pitch, pixel format
        desc->dwHeight = mHeight;
        desc->dwWidth = mWidth;
        desc->lPitch = static_cast<int32_t>(mWidth * (mDepth / 8));
        FillPixelFormat(desc->ddpfPixelFormat, mDepth);
        desc->ddsCaps.dwCaps = mCaps;
    }
    uint32_t mWidth, mHeight, mDepth, mCaps;
    std::vector<uint8_t> mPixels;
    Surface* mBack = nullptr;          // a flipping primary's back buffer
    IDirectDrawClipper* mClipper = nullptr;
};

class Clipper : public IDirectDrawClipper {
public:
    GrafPort* mWindow = nullptr;
};

class DirectDraw : public IDirectDraw4 {
public:
    int32_t QueryInterface(const NS_MacDoze::_GUID&, void** out) override {
        // IDirectDraw2 and IDirectDraw4 are this same object.
        RetainObject(this);
        *out = this;
        return DD_OK;
    }
};

Surface* AsSurface(NS_MacDoze::IUnknown* object) { return static_cast<Surface*>(object); }
}  // namespace

// ---- IUnknown ----------------------------------------------------------------------------
namespace NS_MacDoze {

IUnknown::~IUnknown() {}

int32_t IUnknown::QueryInterface(const _GUID&, void** out) {
    *out = nullptr;
    return E_NOINTERFACE;
}

uint32_t IUnknown::Release() {
    auto& counts = RefCounts();
    const auto it = counts.find(this);
    if (it == counts.end()) return 0;
    if (--it->second != 0) return it->second;
    counts.erase(it);
    delete this;
    return 0;
}

}  // namespace NS_MacDoze

// ---- DirectDraw ----------------------------------------------------------------------------
const NS_MacDoze::_GUID IID_IDirectDraw2 = {{0xe0, 0xf3, 0xa6, 0xb3, 0x43, 0x2b, 0xcf, 0x11, 0xa2, 0xde, 0x00, 0xaa, 0x00,
                                             0xb9, 0x33, 0x56}};
const NS_MacDoze::_GUID IID_IDirectDraw4 = {{0x2a, 0x50, 0x59, 0x9c, 0xbd, 0x39, 0xd1, 0x11, 0x8c, 0x4a, 0x00, 0xc0, 0x4f,
                                             0xd9, 0x30, 0xc5}};

// One device: the display (the NULL GUID, DirectDraw's primary display driver).
int32_t DirectDrawEnumerateA(int32_t (*callback)(NS_MacDoze::_GUID*, char*, char*, void*), void* context) {
    char description[] = "Primary Display Driver";
    char name[] = "display";
    callback(nullptr, description, name, context);
    return DD_OK;
}

int32_t DirectDrawCreate(NS_MacDoze::_GUID*, IDirectDraw** out, NS_MacDoze::IUnknown*) {
    auto* object = new DirectDraw;
    RetainObject(object);
    *out = object;
    if (tpw::MainWindow() != nullptr) {
        int w = 0, h = 0;
        tpw::GameSize(w, h);   // the game's resolution (with the fit the window is another size)
        gMode.width = static_cast<uint32_t>(w);
        gMode.height = static_cast<uint32_t>(h);
    }
    return DD_OK;
}

int32_t IDirectDraw::SetCooperativeLevel(GrafPort*, uint32_t) { return DD_OK; }

// Full screen: the game keeps drawing at its own resolution, which it now names; the window follows
// (tpw::SetGameSize: re-fitted to the screen, or resized to it with --no-fit).
int32_t IDirectDraw::SetDisplayMode(uint32_t width, uint32_t height, uint32_t depth, uint32_t, uint32_t) {
    gMode.width = width;
    gMode.height = height;
    gMode.depth = depth == 16 ? 16 : 32;
    tpw::SetGameSize(static_cast<int>(width), static_cast<int>(height));
    return DD_OK;
}

int32_t IDirectDraw::RestoreDisplayMode() { return DD_OK; }

// The engine reads only dwVidMemTotal (tb_ddraw.cpp SetupCaps); the rest stays zero.
int32_t IDirectDraw::GetCaps(DDCAPS_DX6* driver, DDCAPS_DX6* emulation) {
    for (DDCAPS_DX6* caps : {driver, emulation}) {
        if (caps == nullptr) continue;
        const uint32_t size = caps->dwSize;
        std::memset(caps, 0, sizeof *caps);
        caps->dwSize = size;
        caps->dwVidMemTotal = 64u * 1024 * 1024;
    }
    return DD_OK;
}

int32_t IDirectDraw::CreateClipper(uint32_t, IDirectDrawClipper** out, NS_MacDoze::IUnknown*) {
    auto* clipper = new Clipper;
    RetainObject(clipper);
    *out = clipper;
    return DD_OK;
}

int32_t IDirectDrawClipper::SetHWnd(uint32_t, GrafPort* window) {
    static_cast<Clipper*>(this)->mWindow = window;
    return DD_OK;
}

int32_t IDirectDraw4::GetDeviceIdentifier(DDDEVICEIDENTIFIER* identifier, uint32_t) {
    std::memset(identifier, 0, sizeof *identifier);
    std::strncpy(identifier->driver, "tpw platform layer", sizeof identifier->driver - 1);
    std::strncpy(identifier->description, "OpenGL (SDL3)", sizeof identifier->description - 1);
    return DD_OK;
}

int32_t IDirectDraw4::GetDisplayMode(DDSURFACEDESC2* desc) {
    std::memset(desc, 0, sizeof *desc);
    desc->dwSize = sizeof *desc;
    desc->dwFlags = 0x100e;   // height, width, pitch, pixel format
    desc->dwHeight = gMode.height;
    desc->dwWidth = gMode.width;
    desc->lPitch = static_cast<int32_t>(gMode.width * (gMode.depth / 8));
    FillPixelFormat(desc->ddpfPixelFormat, gMode.depth);
    return DD_OK;
}

// A primary surface (caps 0x200) is the size of the display mode; with back buffers asked
// for (the full-screen flipping chain) one back buffer is attached. Any other surface has
// the size in the description.
int32_t IDirectDraw4::CreateSurface(DDSURFACEDESC2* desc, IDirectDrawSurface4** out, NS_MacDoze::IUnknown*) {
    if (desc == nullptr || out == nullptr) return DDERR_INVALIDPARAMS;
    const uint32_t caps = (desc->dwFlags & 1) != 0 ? desc->ddsCaps.dwCaps : 0;
    const bool primary = (caps & 0x200) != 0;
    const uint32_t width = primary || (desc->dwFlags & 4) == 0 ? gMode.width : desc->dwWidth;
    const uint32_t height = primary || (desc->dwFlags & 2) == 0 ? gMode.height : desc->dwHeight;
    auto* surface = new Surface(width, height, gMode.depth, caps);
    RetainObject(surface);
    if (primary && (desc->dwFlags & 0x20) != 0 && desc->dwBackBufferCount != 0) {
        surface->mBack = new Surface(width, height, gMode.depth, 4);   // DDSCAPS_BACKBUFFER
        RetainObject(surface->mBack);
    }
    *out = surface;
    return DD_OK;
}

// ---- Surfaces ----------------------------------------------------------------------------
int32_t IDirectDrawSurface4::GetAttachedSurface(DDSCAPS2* caps, IDirectDrawSurface4** out) {
    Surface* back = AsSurface(this)->mBack;
    if (back == nullptr || caps == nullptr || (caps->dwCaps & 4) == 0) {
        *out = nullptr;
        return DDERR_NOTFOUND;
    }
    RetainObject(back);
    *out = back;
    return DD_OK;
}

int32_t IDirectDrawSurface4::Lock(SamsUtilities::URectangle::LongRect*, DDSURFACEDESC2* desc, uint32_t,
                                 NS_MacDoze::InternalHandle*) {
    Surface* surface = AsSurface(this);
    surface->Describe(desc);
    desc->lpSurface = surface->mPixels.data();
    return DD_OK;
}

int32_t IDirectDrawSurface::Unlock(void*) { return DD_OK; }

int32_t IDirectDrawSurface::Restore() { return DD_OK; }

int32_t IDirectDrawSurface::SetClipper(IDirectDrawClipper* clipper) {
    AsSurface(this)->mClipper = clipper;
    return DD_OK;
}

int32_t IDirectDrawSurface3::GetSurfaceDesc(DDSURFACEDESC2* desc) {
    AsSurface(this)->Describe(desc);
    return DD_OK;
}

// Only what the engine's start-up asks for: a colour fill of the whole surface (flag
// 0x400, the fill colour in DDBLTFX's last word) and whole-surface copies between surfaces
// of the same size. Nothing is drawn to the screen from here.
int32_t IDirectDrawSurface4::Blt(SamsUtilities::URectangle::LongRect*, IDirectDrawSurface4* source,
                                SamsUtilities::URectangle::LongRect*, uint32_t flags, DDBLTFX* fx) {
    Surface* dest = AsSurface(this);
    if ((flags & 0x400) != 0 && fx != nullptr) {
        const uint32_t colour = fx->words[23];
        if (dest->mDepth == 16) {
            auto* p = reinterpret_cast<uint16_t*>(dest->mPixels.data());
            for (size_t i = 0, n = dest->mPixels.size() / 2; i < n; ++i) p[i] = static_cast<uint16_t>(colour);
        } else {
            auto* p = reinterpret_cast<uint32_t*>(dest->mPixels.data());
            for (size_t i = 0, n = dest->mPixels.size() / 4; i < n; ++i) p[i] = colour;
        }
        return DD_OK;
    }
    if (source != nullptr) {
        const Surface* from = AsSurface(source);
        if (from->mPixels.size() == dest->mPixels.size())
            std::memcpy(dest->mPixels.data(), from->mPixels.data(), dest->mPixels.size());
    }
    return DD_OK;
}
