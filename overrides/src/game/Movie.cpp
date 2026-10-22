// Movie.cpp: guessed name (the binary has no __sinit for this file): a contiguous range of free functions, named after what they do.

#include "FS_System.h"
#include "platform/samsutils.h"
#include "rsse.h"
#include "TPMain.h"
#include "TPMovie.h"

// TPW_PORT_FIX wfilm1 MOVIE_Play | Mac: the film spans the screen's width, 352/480 of its height high, 64/480 down | Port: on a screen wider than 4:3 the 640 x 352 picture fills the screen (centred, a little cropped) or, when that would crop more than 15 percent, fits inside it (include/platform/filmbox.h) | Why: a widescreen port; the film no longer sits in a box with bars on all four sides
#include "UI_Base.h"
#include "platform/filmbox.h"

// The movie player's range of the main app (SimTheme Park 2.04b3; the inventory files it
// under rsse.cpp): MOVIE_Play 0x100b9914, MOVIE_Tick 0x100b9bcc, MOVIE_Stop 0x100b9c48,
// MOVIE_SetVolume 0x100b9cbc, MOVIE_Init 0x100b9d10, MOVIE_Close 0x100b9d58, and their
// neighbour CheckFileExists (0x100b98a4). One movie at a time, played by QuickTime into the
// game's window.

Movie gMovie;   // 0x102e49ec (name invented)

uint32_t gMovieVolume = 0x100;   // 0x1029eb88 (name invented): 0 .. 0x100, full at start (the image's word)

// Every step that fails reports its error and ends in "bailed out"; MOVIE_Close then
// leaves QuickTime (ExitMovies) although nothing enters it again before the next movie.
int32_t MOVIE_Play(const char* path, GrafPort* window, int32_t x, int32_t y, int32_t) {
    unsigned char name[256];
    FSSpec spec;
    bool targetIsFolder;
    bool wasAliased;
    int16_t file;
    SamsUtilities::UString::c2pcpy(name, path);
    if (gMovie != nullptr) MOVIE_Close();
    OSErr error = FSMakeFSSpec(0, 0, name, &spec);
    if (error == 0) error = ResolveAliasFile(&spec, true, &targetIsFolder, &wasAliased);
    if (error == 0) error = OpenMovieFile(&spec, &file, 1);
    if (error == 0) {
        int16_t resource = 0;
        error = NewMovieFromFile(&gMovie, file, &resource, nullptr, 1, nullptr);
    }
    if (error != 0) {
        SamsUtilities::UDebug("bailed: error %d", error);
        SamsUtilities::UDebug("bailed out");
        MOVIE_Close();
        return 0;
    }
    Rect box;
    GetMovieBox(gMovie, &box);
    SetPort(window);
    ShowWindow(window);
    ValidRect(&box);
    SetMovieGWorld(gMovie, window, GetGWorldDevice(window));
    // On a screen wider than 4:3 (gUIUnitWidth past 0x800) the film's box is the helper's. The callers' x and y are the
    // window's origin (in screen coordinates, which ToLocal takes off again) and, in y, the letterbox offset MovieTop()
    // (gScreenHeight * 64 / 480); the box places itself, so only the origin is kept. Otherwise the Mac's box.
    int32_t left = x, right = x + gScreenWidth;
    int32_t top = y, bottom = y + static_cast<int32_t>(static_cast<uint32_t>(gScreenHeight * 0x160) / 0x1e0);
    if (gUIUnitWidth > 0x800) {
        const FilmBox film = FilmBoxFor(gScreenWidth, gScreenHeight);
        const int32_t origin = y - static_cast<int32_t>((static_cast<uint32_t>(gScreenHeight) << 6) / 480);
        left = x + film.left;
        right = x + film.right;
        top = origin + film.top;
        bottom = origin + film.bottom;
    }
    SamsUtilities::URectangleMac place(left, top, right, bottom);
    place.ToLocal(window);
    SetMovieBox(gMovie, place);
    SetMovieVolume(gMovie, static_cast<int16_t>(gMovieVolume));
    for (int32_t i = 0; i < 10; ++i) MoviesTask(gMovie, 0);
    const TimeValue time = GetMovieTime(gMovie, nullptr);
    const Fixed rate = GetMoviePreferredRate(gMovie);
    PrerollMovie(gMovie, time, rate);
    SetMovieRate(gMovie, rate);
    return 1;
}

int32_t MOVIE_Tick() {
    if (gMovie == nullptr) return 1;
    MoviesTask(gMovie, 0);
    return IsMovieDone(gMovie);
}

void MOVIE_Stop() {
    if (gMovie != nullptr) {
        StopMovie(gMovie);
        DisposeMovie(gMovie);
        gMovie = nullptr;
    }
}

// The game's 0 .. 0x3ff scaled to 0 .. 0x100.
void MOVIE_SetVolume(uint32_t volume) { gMovieVolume = (volume * 0x100) / 0x3ff; }

void MOVIE_Init() { EnterMovies(); }

void MOVIE_Close() {
    if (gMovie != nullptr) {
        StopMovie(gMovie);
        DisposeMovie(gMovie);
        gMovie = nullptr;
    }
    ExitMovies();
}
