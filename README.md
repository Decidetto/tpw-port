# tpw-port

A native port of Theme Park World (Sim Theme Park), built on the [tpw-decomp](https://gitlab.com/tpw-decomp/tpw-decomp) translation of the
1999 Mac build. It runs on macOS with SDL3 and OpenGL.

tpw-decomp is the reference: the game's own functions, each behaving exactly as on the Mac, bugs included. This repository
adds what it takes to play:

- launcher/: the program (window, options, the game's start-up sequence) and the generated stand-ins for any function not
  translated yet;
- platform/: the platform layer, the Mac Toolbox, Sound Sprockets, QuickTime and the game's Windows-style shims done on
  SDL3, OpenGL and CoreText;
- overrides/: the port's fixes, 1 so far, each one listed in [FIXES.md](FIXES.md) with what the Mac does, what the port does
  and why. Each file there is built in place of the tpw-decomp file of the same path; the rest of tpw-decomp is built as
  it is.

## Nothing from the game

Neither repository holds any part of the game. You need your own copy: the 2.04b3 patch's "SimTheme Park" application
(the build reads six data tables from it) and the CD's "Theme Park Data" folder, which the game reads at run time. See
tpw-decomp's README for the folder layout and devtools/hfsdump.py, which unpacks the CD's HFS partition.

    WORK/
      tpw-port/                        this repository (tpw-port/tpw-decomp is the submodule)
      extracted/
        SimTPPatch_204b3/Sim Theme Park Patch/patch 2.04b3/SimTheme Park
        disc-hfs/Theme Park Data/

## Building

Needs CMake 3.20, a C++17 compiler, zlib, Python 3, SDL3 and OpenGL; FFmpeg's libraries for the films (optional, the game
skips its films without them).

    git clone --recurse-submodules <tpw-port>
    cmake -S tpw-port -B build -DTPW_GAME_PROGRAM="<the 2.04b3 SimTheme Park application>"
    cmake --build build -j8 --target tpw_launcher

The submodule's URL is relative (../tpw-decomp), so it resolves to tpw-decomp beside tpw-port, on the same server or in
the same folder. A clone of a local folder needs `git -c protocol.file.allow=always submodule update --init`.

## Playing

    devtools/run_launcher.sh build NAME SECONDS [options]

runs build/tpw_launcher from the working folder with its own user folder (build/user-NAME) and log (build/launch-NAME.log)
and quits it after SECONDS. Options: --mute, --no-movies, --windowed, --display builtin, --background, and the scripted
input used to test it (--click X,Y@PASS, --type TEXT@PASS, --drag X0,Y0-X1,Y1@PASS, --frames N).
build/tpw_platform_tests checks the platform layer without a display.

## See also

- [tpw-decomp](https://gitlab.com/tpw-decomp/tpw-decomp): the translation and its differential tests.
- [FIXES.md](FIXES.md): every place the port behaves differently from the Mac.

No license has been chosen yet.
