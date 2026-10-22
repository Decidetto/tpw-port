// window.h: the launcher's own window: about 800 x 560 points, resizable, sharp on high-density screens, running the
// screens of launcher_ui.h until the player presses Play or closes it. The window is gone again when this returns, so
// the game can open its own. launcher/main.cpp opens it when no --game is given.
#pragma once
#include <string>

#include "backend.h"
#include "launcher_ui.h"

namespace tpw::ui {

enum class WindowOutcome {
    Play,    // `choice` says what to start
    Quit,    // the window was closed
    Failed,  // `err` says why
};

// Where the window opens: centred on an SDL display (0: SDL's choice), and in front or, for a test run on a screen
// someone is working on, behind the other windows without taking the focus (the launcher's --display and --background).
struct WindowPlacement {
    SDL_DisplayID display = 0;
    bool background = false;
};

// Initialises SDL's video subsystem if it is not yet, and leaves it initialised.
WindowOutcome RunLauncherWindow(ImportBackend& backend, LaunchChoice& choice, std::string& err,
                                const WindowPlacement& placement = {});

}  // namespace tpw::ui
