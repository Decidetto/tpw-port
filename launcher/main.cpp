// main.cpp: the native launcher. It points the platform layer at the installed game and at
// a folder for what the game writes, then runs the game's own main (ThemeParkWorldMain,
// TPMain.cpp). Whatever is not translated or implemented yet is a generated stand-in that
// reports itself; a summary of the ones that were hit is printed at the end.
#include <SDL3/SDL.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>

#include <dlfcn.h>
#include <execinfo.h>
#include <sys/ucontext.h>
#include <unistd.h>

#include "TPMain.h"
#include "tpw_probe.h"
#include "ResolutionBalance.h"
#include "UI_Base.h"
#include "platform.h"
#include "static_init.h"
#include "stubs.h"

namespace {
// The largest preset, 4096 x 3072 (3840 x 2160 at 16:9), rendered offscreen and scaled to the window (the maintainer,
// 2026-10-20; it was Resolution.sam's 2048 x 1536 preset, 7, from 2026-10-11). Presets 8 to 10 are the port's own.
constexpr int32_t kDefaultResolution = 10;
// The screen's shape: 16:9 unless --aspect asks otherwise (the maintainer, 2026-10-20). Presets 1 to 7 keep their
// height and take the width the shape gives them, so 2048 x 1536 becomes 2730 x 1536; presets 8 to 10 keep the width of
// their 16:9 size (2560, 3200, 3840) and take the height. 4:3 is the original game's layout exactly.
constexpr int kDefaultAspectWide = 16;
constexpr int kDefaultAspectHigh = 9;
bool gAbortOnStub = false;
bool gTraceStubs = false;
long gStubCalls = 0;
const char* volatile gLastStub = nullptr;
// Calls per stand-in (keyed by its name, which the stub table keeps for the whole run), so the
// report can name the ones the game leans on.
std::unordered_map<const char*, long> gStubCallsByName;

// Apply the native override after the game's balance files and saved settings load,
// before the first Process call chooses fonts and the screen size. The translated
// start-up and the original entry point remain unchanged.
class ResolutionOverrideWorld final : public ThemeParkWorld {
public:
    explicit ResolutionOverrideWorld(int32_t preset) : mPreset(preset) {}
    bool Initialise() override {
        if (!ThemeParkWorld::Initialise()) return false;
        gResolutionBalance.mValues[1].mInteger = mPreset;
        return true;
    }
private:
    int32_t mPreset;
};

int ResolutionPreset(const char* size) {
    const char* sizes[] = {"512x384",   "640x480",   "800x600",   "1024x768",  "1280x1024",
                           "1600x1200", "2048x1536", "2560x1920", "3200x2400", "4096x3072"};
    for (int i = 0; i < 10; ++i)
        if (std::strcmp(size, sizes[i]) == 0) return i + 1;
    return 0;
}

// W:H with both whole and positive, no narrower than 4:3; 0 for anything else.
float ParseAspect(const char* text) {
    int wide = 0;
    int high = 0;
    char end = 0;
    if (std::sscanf(text, "%d:%d%c", &wide, &high, &end) != 2 || wide <= 0 || high <= 0 || wide * 3 < high * 4) return 0.0f;
    return static_cast<float>(wide) / static_cast<float>(high);
}

#ifdef TPW_PORT_FIX
// The largest preset, from `preset` down, whose size at the launcher's aspect this machine's OpenGL can render (limit:
// tpw::LargestGameSize, 0 when unknown); said once in the log when it is not `preset`.
int32_t FittingPreset(int32_t preset, int limit) {
    if (preset <= 0 || limit <= 0) return preset;
    int32_t chosen = preset;
    int32_t wide = 0;
    int32_t high = 0;
    UI_PresetSize(chosen, gUIWideAspect, wide, high);
    while (chosen > 1 && (wide > limit || high > limit)) UI_PresetSize(--chosen, gUIWideAspect, wide, high);
    if (chosen != preset) {
        int32_t wanted = 0;
        int32_t wantedHigh = 0;
        UI_PresetSize(preset, gUIWideAspect, wanted, wantedHigh);
        tpw::Log("resolution: %dx%d is larger than this machine's %d-pixel framebuffers; %dx%d (preset %d) instead", wanted,
                 wantedHigh, limit, wide, high, chosen);
    }
    return chosen;
}
#endif

int RunGame(int32_t resolution) {
    // What the Mac's runtime did before main (launcher/static_init.cpp).
    RunStaticInitialisers();
    if (resolution == 0) return ThemeParkWorldMain();
    ResolutionOverrideWorld world(resolution);
    world.Run();
    return 0;
}

// A crash right after a stand-in returned is usually caused by it (a zero where the caller
// expected an object, or a structure result left unwritten): name the last one reached.
void OnCrash(int signal) {
    static char text[512];
    const int length = std::snprintf(text, sizeof text, "launcher: crashed (signal %d); the last stand-in reached was: %s\n",
                                     signal, gLastStub != nullptr ? gLastStub : "(none)");
    if (length > 0) (void)!write(2, text, static_cast<size_t>(length));
    if (const char* running = RunningStaticInitialiser()) {
        const int more = std::snprintf(text, sizeof text, "launcher: inside the static initialiser %s\n", running);
        if (more > 0) (void)!write(2, text, static_cast<size_t>(more));
    }
    // Where it happened (the launcher keeps its symbols).
    static void* frames[48];
    backtrace_symbols_fd(frames, backtrace(frames, 48), 2);
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

// The faulting instruction and address first: the backtrace's frame after _sigtramp is the caller when the fault is in a
// function without a frame record (a leaf), so it can name the wrong place.
void OnCrashInfo(int signal, siginfo_t* info, void* context) {
    uintptr_t pc = 0;
#if defined(__arm64__) || defined(__aarch64__)
    pc = static_cast<uintptr_t>(static_cast<ucontext_t*>(context)->uc_mcontext->__ss.__pc);
#elif defined(__x86_64__)
    pc = static_cast<uintptr_t>(static_cast<ucontext_t*>(context)->uc_mcontext->__ss.__rip);
#endif
    Dl_info where{};
    const bool named = dladdr(reinterpret_cast<void*>(pc), &where) != 0 && where.dli_sname != nullptr;
    static char text[512];
    const int length = std::snprintf(text, sizeof text, "launcher: fault at address %p, instruction %p (%s + %ld)\n", info->si_addr,
                                     reinterpret_cast<void*>(pc), named ? where.dli_sname : "?",
                                     named ? static_cast<long>(pc - reinterpret_cast<uintptr_t>(where.dli_saddr)) : 0L);
    if (length > 0) (void)!write(2, text, static_cast<size_t>(length));
    OnCrash(signal);
}

void Usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s [options]\n"
                 "  --game DIR     the installed game: the folder that holds \"data\"\n"
                 "                 (default: $TPW_GAME_DIR, else \"Theme Park Data\" here)\n"
                 "  --user DIR     where saves and settings go (default: a per-user folder)\n"
                 "  --windowed     run in a window instead of full screen\n"
                 "  --resolution WxH  the game's resolution, named by its 4:3 size: a Resolution.sam preset (512x384,\n"
                 "                    640x480, 800x600, 1024x768, 1280x1024, 1600x1200, 2048x1536) or a larger one of the\n"
                 "                    port's (2560x1920, 3200x2400, 4096x3072; default: 4096x3072). The game renders at it\n"
                 "                    offscreen and the window shows it scaled\n"
                 "  --aspect W:H   the screen's shape (default 16:9). The Resolution.sam presets keep their height and are\n"
                 "                 widened (2048x1536 becomes 2730x1536 at 16:9, 2458x1536 at 16:10); the larger ones keep\n"
                 "                 the width of their 16:9 size (2560x1440, 3200x1800, 3840x2160 at 16:9; 3840x2400 at\n"
                 "                 16:10). The park is seen further to the sides, the panels keep to the screen's edges\n"
                 "                 and the 4:3 screens are pillarboxed. --aspect 4:3 is the original layout exactly, at\n"
                 "                 the 4:3 size\n"
                 "  --no-intro     skip the logo, the intro movie and the lobby: straight into a park\n"
                 "  --frames N     stop after N passes of the main loop\n"
                 "  --click X,Y@N  a scripted left click at game pixel (X, Y): pressed on pass N, released on pass N+1\n"
                 "                 (repeatable; the pointer stays there until the real mouse moves)\n"
                 "  --drag X0,Y0-X1,Y1@N[/K]  a scripted left drag: the pointer to (X0, Y0) on pass N, the button down on\n"
                 "                 pass N+2, K steps (default 8) to (X1, Y1), the button up on pass N+K+4 (repeatable)\n"
                 "  --type TEXT@N  scripted typing: one character every two passes from pass N (letters, digits, space;\n"
                 "                 \\r for return)\n"
                 "  --no-movies    skip the logo and intro films\n"
                 "  --mute         no sound\n"
                 "  --display D    open the window on screen D: an SDL display number (0, 1, ...) or part of the\n"
                 "                 screen's name, case-blind (\"builtin\" is the MacBook's own screen; the log lists them)\n"
                 "  --no-fit       a window as large as the game's resolution in points, drawn into directly (by default the\n"
                 "                 window is fitted to the screen and the game renders at its own resolution offscreen)\n"
                 "  --background   open the window behind the others, without taking the focus or moving the mouse\n"
                 "                 (a test run on a screen someone is working on; a click focuses it as usual)\n"
                 "  --dump-frames DIR  save the presented frames there as frame-NNNN.bmp (each one that differs from\n"
                 "                     the frame before it; the first 200)\n"
                 "  --dump-every N     with --dump-frames: save only every Nth of those frames\n"

                 "  --stub-abort   abort at the first unimplemented function (for a debugger)\n"
                 "  --stub-trace   log every call of an unimplemented function, not only the first\n",
                 program);
}

void Report() {
    std::vector<const char*> names(static_cast<size_t>(tpw_stub_count()) + 1);
    const int hit = tpw_stub_hits(names.data(), static_cast<int>(names.size()));
    std::fprintf(stderr, "launcher: %d of %d stand-ins were reached (%ld calls in all)\n", hit, tpw_stub_count(),
                 gStubCalls);
    std::vector<std::pair<long, const char*>> byCalls;
    for (const auto& entry : gStubCallsByName) byCalls.emplace_back(entry.second, entry.first);
    std::sort(byCalls.begin(), byCalls.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    for (const auto& entry : byCalls) std::fprintf(stderr, "launcher: %10ld calls  %s\n", entry.first, entry.second);
}
}  // namespace

void tpw_stub_called(const char* name, bool platform, bool first) {
    ++gStubCalls;
    ++gStubCallsByName[name];
    gLastStub = name;
    if (first || gTraceStubs)
        tpw::LogLine(platform ? "unimplemented platform call: " : "not translated yet: ", name);
    if (gAbortOnStub) std::abort();
}

int main(int argc, char** argv) {
    // a trap (a hardened library check), an illegal instruction and an abort (a failed stack check is a silent one) get
    // the backtrace too
    for (int crash : {SIGSEGV, SIGBUS, SIGTRAP, SIGILL, SIGABRT}) {
        struct sigaction action{};
        action.sa_sigaction = OnCrashInfo;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        sigaction(crash, &action, nullptr);
    }
    tpw::Config& config = tpw::config();
    int32_t resolution = kDefaultResolution;
    int aspectWide = kDefaultAspectWide;
    int aspectHigh = kDefaultAspectHigh;
    if (const char* dir = std::getenv("TPW_GAME_DIR")) config.gameDir = dir;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> const char* {
            if (i + 1 >= argc) {
                Usage(argv[0]);
                std::exit(2);
            }
            return argv[++i];
        };
        if (arg == "--game") config.gameDir = value();
        else if (arg == "--user") config.userDir = value();
        else if (arg == "--windowed") config.debugOptions[1] = 0;
        else if (arg == "--resolution") {
            const char* size = value();
            resolution = ResolutionPreset(size);
            if (resolution == 0) {
                std::fprintf(stderr, "launcher: unsupported resolution: %s\n", size);
                Usage(argv[0]);
                return 2;
            }
            tpw::Log("resolution override: %s", size);
        }
        else if (arg == "--aspect") {
            const char* shape = value();
            if (ParseAspect(shape) == 0.0f || std::sscanf(shape, "%d:%d", &aspectWide, &aspectHigh) != 2) {
                std::fprintf(stderr, "launcher: --aspect wants W:H, no narrower than 4:3 (16:9, 16:10, 4:3): %s\n", shape);
                return 2;
            }
        }
        else if (arg == "--no-intro") config.debugOptions[3] = 0;
        else if (arg == "--frames") config.maxFrames = std::atoi(value());
        else if (arg == "--click") {
            const char* spec = value();
            tpw::Config::ScriptedClick click{};
            if (std::sscanf(spec, "%d,%d@%d", &click.x, &click.y, &click.pass) != 3 || click.pass < 1) {
                std::fprintf(stderr, "launcher: --click wants X,Y@N (game pixel and pass number from 1): %s\n", spec);
                return 2;
            }
            config.clicks.push_back(click);
        }
        else if (arg == "--drag") {
            const char* spec = value();
            tpw::Config::ScriptedDrag drag{};
            drag.steps = 8;
            const int read = std::sscanf(spec, "%d,%d-%d,%d@%d/%d", &drag.x0, &drag.y0, &drag.x1, &drag.y1, &drag.pass, &drag.steps);
            if (read < 5 || drag.pass < 1 || drag.steps < 1) {
                std::fprintf(stderr, "launcher: --drag wants X0,Y0-X1,Y1@N or X0,Y0-X1,Y1@N/STEPS: %s\n", spec);
                return 2;
            }
            config.drags.push_back(drag);
        }
        else if (arg == "--type") {
            const std::string spec = value();
            const size_t at = spec.rfind('@');
            tpw::Config::ScriptedTyping typing{};
            if (at == std::string::npos || (typing.pass = std::atoi(spec.c_str() + at + 1)) < 1) {
                std::fprintf(stderr, "launcher: --type wants TEXT@N (the text and the pass number from 1): %s\n", spec.c_str());
                return 2;
            }
            const std::string raw = spec.substr(0, at);
            for (size_t i = 0; i < raw.size(); ++i) {   // the two characters "\\r" stand for return
                if (raw[i] == '\\' && i + 1 < raw.size() && raw[i + 1] == 'r') {
                    typing.text += '\r';
                    ++i;
                } else {
                    typing.text += raw[i];
                }
            }
            config.typing.push_back(typing);
        }
        else if (arg == "--probe") {
            config.probe = true;
            tpw_probe::gLog = [](const char* text) { tpw::Log("%s", text); };
        }
        else if (arg == "--dump-frames") config.dumpFramesDir = value();
        else if (arg == "--dump-every") config.dumpEvery = std::atoi(value());
        else if (arg == "--dump-limit") config.dumpLimit = std::atoi(value());
        else if (arg == "--no-movies") config.movies = false;
        else if (arg == "--mute") config.mute = true;
        else if (arg == "--display") config.display = value();
        else if (arg == "--background") config.background = true;
        else if (arg == "--no-fit") config.fit = false;
        else if (arg == "--stub-abort") gAbortOnStub = true;
        else if (arg == "--stub-trace") gTraceStubs = true;
        else {
            Usage(argv[0]);
            return arg == "--help" || arg == "-h" ? 0 : 2;
        }
    }
    if (config.gameDir.empty()) config.gameDir = "Theme Park Data";
    std::error_code ec;
    if (!std::filesystem::is_directory(config.gameDir / "data", ec)) {
        std::fprintf(stderr, "launcher: no game found in \"%s\" (it must hold the game's \"data\" folder)\n",
                     config.gameDir.string().c_str());
        return 1;
    }
    if (config.userDir.empty()) {
        if (char* pref = SDL_GetPrefPath("tpw-decomp", "Theme Park World")) {
            config.userDir = pref;
            SDL_free(pref);
        } else {
            config.userDir = "tpw-user";
        }
    }
    std::filesystem::create_directories(config.userDir, ec);
    tpw::Log("game folder: %s", config.gameDir.string().c_str());
    tpw::Log("user folder: %s", config.userDir.string().c_str());

    // 4:3 leaves the game's own sizes alone (the 1280 x 1024 preset stays 5:4); anything wider widens the screen.
#ifdef TPW_PORT_FIX
    gUIWideAspect = aspectWide * 3 == aspectHigh * 4 ? 0.0f : static_cast<float>(aspectWide) / static_cast<float>(aspectHigh);
    tpw::Log("aspect: %d:%d%s", aspectWide, aspectHigh, gUIWideAspect == 0.0f ? " (the original 4:3 layout)" : "");
    gUIHighPresets = 1;
    // The preset is the size the game renders at, not the window's; a machine whose framebuffers are smaller gets the
    // largest preset that fits. TPW_LARGEST_GAME_SIZE stands in for the machine's limit, to check the fallback.
    if (resolution > 0) {
        const char* forced = std::getenv("TPW_LARGEST_GAME_SIZE");
        resolution = FittingPreset(resolution, forced != nullptr ? std::atoi(forced) : tpw::LargestGameSize());
    }
#else
    tpw::Log("aspect: 4:3 (a build without the port's fixes has the original layout only)");
    if (resolution > 7) {
        tpw::Log("resolution: a build without the port's fixes has Resolution.sam's presets only; 2048x1536");
        resolution = 7;
    }
#endif

    const int result = RunGame(resolution);
    Report();
    SDL_Quit();
    return result;
}
