// window.cpp: the launcher's own window (window.h).
#include "window.h"

#include <algorithm>

#include "theme.h"

namespace tpw::ui {

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
        while (running) {
            // Rest while nothing moves; wake for events (file dialogs push one when they close).
            SDL_Event e;
            bool got = ui.Animating() ? SDL_PollEvent(&e) : SDL_WaitEventTimeout(&e, 250);
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
            SDL_RenderPresent(renderer);
            if (ui.TakePlay(choice)) {
                outcome = WindowOutcome::Play;
                running = false;
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
