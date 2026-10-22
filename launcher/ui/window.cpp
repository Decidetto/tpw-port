// window.cpp: the launcher's own window (window.h).
#include "window.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

#include "theme.h"

namespace tpw::ui {

namespace {

// The frame just drawn (before it is presented), saved as frame-NNNN.bmp when it differs from the one before it, as the
// game's window does (platform/window.cpp DumpFrame): every `every`'th of the differing frames, at most `limit` pictures.
class FrameDumper {
public:
    FrameDumper(const WindowPlacement& placement)
        : dir_(placement.dumpDir), every_(std::max(placement.dumpEvery, 1)), limit_(std::max(placement.dumpLimit, 0)) {}

    void Dump(SDL_Renderer* renderer) {
        if (dir_.empty() || count_ >= limit_) return;
        SDL_Surface* shot = SDL_RenderReadPixels(renderer, nullptr);
        if (!shot) return;
        SDL_Surface* rgb = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGB24);
        SDL_DestroySurface(shot);
        if (!rgb) return;
        const size_t rowBytes = static_cast<size_t>(rgb->w) * 3;
        std::vector<uint8_t> bytes(rowBytes * static_cast<size_t>(rgb->h));
        for (int row = 0; row < rgb->h; ++row)
            std::memcpy(&bytes[rowBytes * static_cast<size_t>(row)], static_cast<const uint8_t*>(rgb->pixels) + static_cast<size_t>(row) * rgb->pitch, rowBytes);
        if (bytes != last_ && seen_++ % every_ == 0) {
            char name[32];
            std::snprintf(name, sizeof name, "frame-%04d.bmp", count_++);
            std::error_code ec;
            std::filesystem::create_directories(dir_, ec);
            if (SDL_SaveBMP(rgb, (std::filesystem::path(dir_) / name).string().c_str()))
                std::fprintf(stderr, "launcher: dumped %s (frame %d)\n", name, frames_);
            else
                std::fprintf(stderr, "launcher: could not save %s: %s\n", name, SDL_GetError());
        }
        last_ = std::move(bytes);
        SDL_DestroySurface(rgb);
    }
    void NextFrame() { ++frames_; }

private:
    std::string dir_;
    int every_, limit_;
    int count_ = 0, seen_ = 0, frames_ = 0;
    std::vector<uint8_t> last_;
};

}  // namespace

WindowOutcome RunLauncherWindow(ImportBackend& backend, LaunchChoice& choice, std::string& err,
                                const WindowPlacement& placement) {
    if (placement.background) {
        // Shown without becoming the key window (as the game's own window does with --background).
        SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "0");
        SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "0");
    }
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        err = std::string("cannot start SDL video: ") + SDL_GetError();
        return WindowOutcome::Failed;
    }
    // Made hidden, placed, then shown, so it never appears on another screen first.
    SDL_Window* window = SDL_CreateWindow(kProductName, metric::kStartWidth, metric::kStartHeight,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
    if (!window) {
        err = std::string("cannot open the launcher window: ") + SDL_GetError();
        return WindowOutcome::Failed;
    }
    SDL_SetWindowMinimumSize(window, metric::kMinWidth, metric::kMinHeight);
    if (placement.display != 0)
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(placement.display),
                              SDL_WINDOWPOS_CENTERED_DISPLAY(placement.display));
    SDL_ShowWindow(window);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        err = std::string("cannot draw in the launcher window: ") + SDL_GetError();
        SDL_DestroyWindow(window);
        return WindowOutcome::Failed;
    }
    SDL_SetRenderVSync(renderer, 1);

    WindowOutcome outcome = WindowOutcome::Quit;
    {
        LauncherUi ui(renderer, backend);
        ui.SetWindow(window);
        if (!ui.Ok()) {
            err = ui.Error();
            outcome = WindowOutcome::Failed;
        }
        uint64_t last = SDL_GetTicksNS();
        bool running = ui.Ok();
        FrameDumper dumper(placement);
        int drawn = 0;
        while (running) {
            // Rest while nothing moves; wake for events (file dialogs push one when they close).
            SDL_Event e;
            // (a counted test run does not wait long: its frames are what it is after)
            bool got = ui.Animating() ? SDL_PollEvent(&e) : SDL_WaitEventTimeout(&e, placement.maxFrames > 0 ? 16 : 250);
            while (got) {
                if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
                ui.HandleEvent(e);
                got = SDL_PollEvent(&e);
            }
            if (!running) break;
            uint64_t now = SDL_GetTicksNS();
            float dt = static_cast<float>(now - last) / 1e9f;
            last = now;
            int w = 0, h = 0;
            SDL_GetWindowSize(window, &w, &h);
            float density = SDL_GetWindowPixelDensity(window);
            ui.Frame(std::min(dt, 0.1f), static_cast<float>(w), static_cast<float>(h), density > 0 ? density : 1);
            dumper.NextFrame();
            dumper.Dump(renderer);
            SDL_RenderPresent(renderer);
            if (ui.TakePlay(choice)) {
                outcome = WindowOutcome::Play;
                running = false;
            } else if (placement.maxFrames > 0 && ++drawn >= placement.maxFrames) {
                running = false;   // --frames N: the window run ends by itself, exit 0 (Quit)
            }
        }
        // An import still running is cancelled and joined by the importer when its owner goes.
        if (backend.Running()) {
            backend.Cancel();
            backend.Wait();
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return outcome;
}

}  // namespace tpw::ui
