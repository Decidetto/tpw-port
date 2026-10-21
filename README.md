# tpw-port

A native port of Theme Park World (Sim Theme Park), built on the [tpw-decomp](https://gitlab.com/tpw-decomp/tpw-decomp) translation of the
1999 Mac build. It runs on macOS with SDL3 and OpenGL.

tpw-decomp is the reference: the game's own functions, each behaving exactly as on the Mac, bugs included. This repository
adds what it takes to play:

- launcher/: the program (window, options, the game's start-up sequence) and the generated stand-ins for any function not
  translated yet;
- platform/: the platform layer, the Mac Toolbox, Sound Sprockets, QuickTime and the game's Windows-style shims done on
  SDL3, OpenGL and CoreText;
- overrides/: the port's fixes, 42 so far, each one listed in [FIXES.md](FIXES.md) with what the Mac does, what the port does
  and why. Each file there is built in place of the tpw-decomp file of the same path; the rest of tpw-decomp is built as
  it is. The headers are read from a copy of tpw-decomp's headers in the build folder with the overridden ones laid over
  it, so every file of both repositories sees the port's version of a header.

## Nothing from the game

Neither repository holds any part of the game. You need your own copy: the 2.04b3 patch's "SimTheme Park" application
(the build reads six data tables from it) and the CD's "Theme Park Data" folder, which the game reads at run time.

## Game data

The port runs from the original downloads, unchanged. devtools/import_originals.py finds them by their MD5 (any file
names), unpacks them and checks every file it writes against the SHA-256 list it carries:

| Original | MD5 |
| --- | --- |
| From the US Mac CD (SimThemePark Mac US CD v2.0.1): the zip holding the CD image "Theme Park.toast" | f03abce3f375bc72d863cc527749e645 |
| From the patches item (SimThemePark Mac Patches and Updaters): the 2.04b3 patch, a .sea.sit | 23bcbf4d6d6aec0b5ab00d47840b3f63 |
| From the demo and bonus item (ThemeParkWorld Mac Demo and Bonus Stuff): the Bonus Stuff zip; optional, its fonts serve only the platform tests | 1adeba0b0cf5edcc623d44181a321f28 |
| From the UK PC CD item (ThemeParkWorld PC UK-European CD Bullfrog EA): either ISO image; optional, for the Theme Park World edition (devtools/import_eur.py writes extracted/tpw-eur) | df7d864394ad46860984be60f2a3e448 or 50ffee97f9abc37e751f36d7239caea4 |

It needs machfs, from pip, for the CD's HFS volume and unar for the StuffIt archive (macOS: `brew install unar`; Debian
and Ubuntu: `apt install unar`). Put the downloads in one folder, then from WORK, the folder holding tpw-port:

    python3 -m venv venv
    venv/bin/python -m pip install machfs
    venv/bin/python tpw-port/devtools/import_originals.py --from DOWNLOADS --to extracted

It takes about half a minute and 2 GB of memory, writes nothing outside extracted/ and ends with "CHECK OK" (or the list
of the files that differ). `--check extracted` checks the folders again later; `--whole-disc` also writes the rest of the
CD (manuals, installers, extras), which nothing reads. The result:

    WORK/
      tpw-port/                        this repository (tpw-port/tpw-decomp is the submodule)
      extracted/
        SimTPPatch_204b3/Sim Theme Park Patch/patch 2.04b3/SimTheme Park
        disc-hfs/Theme Park Data/      each file's data fork, with NAME.rsrc (its resource fork) and NAME.finfo
        Theme_Park_World_Bonus_Stuff/  the bonus fonts, when that zip is there

The build finds the program there by default (TPW_GAME_PROGRAM names another), and from WORK the game runs with
`build/tpw_launcher --game "extracted/disc-hfs/Theme Park Data"` (or devtools/run_launcher.sh, below).

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

## Widescreen and resolution

The port is widescreen by default: 16:9 at 3840 x 2160. The game renders offscreen at that size and the window shows it
scaled to fit the screen, so a small window still gets a sharp picture (3D edges and the TrueType text; the game's own
textures are 256 x 256 at most and do not gain detail). The game's bitmap fonts have one size for every resolution from
1024 x 768 up, so the panels' small print is smaller on the screen at the larger sizes (at 3840 x 2160 about 0.7 times
its size at 2730 x 1536); --resolution 2048x1536 keeps the size the original gives at its largest preset. The park and the lobby are drawn out to the new edges with the same vertical
field of view, so the sides show more of the park and nothing is cut off; the panels on the park view are anchored to the
edges of the screen (the money and the control panel left, the advisor right, dialogs in the middle); the 4:3 art (the
logo, the loading screens, the films and the full-screen pictures) keeps its shape in the middle of the screen with black
bars at the sides. The window fits the screen at that aspect, so a 16:9 screen shows no bars round the game. A machine
whose framebuffers or textures are smaller than the size asked for gets the largest preset that fits, and the log says
so once (TPW_LARGEST_GAME_SIZE=3000 in the environment tries that on a machine that has room).

    tpw_launcher --resolution 2048x1536   a smaller size, named by its 4:3 size (the game's own presets from 512x384 to
                                          2048x1536, which keep their height when widened: 2730 x 1536 at 16:9; or
                                          the port's 2560x1920, 3200x2400 and 4096x3072, which keep the width of their
                                          16:9 size: 2560 x 1440, 3200 x 1800, 3840 x 2160)
    tpw_launcher --aspect 16:10           another shape (anything from 4:3 wider: 3840 x 2400 at the default size)
    tpw_launcher --aspect 4:3             the original layout, exactly as the Mac draws it (4096 x 3072 by default)

devtools/run_launcher.sh passes --aspect 4:3 and --resolution 2048x1536 unless its options name them, because the
scripted clicks of the test recipes are 4:3 game pixels at 2048 x 1536; a widescreen test run passes --aspect 16:9 and
moves its clicks on right-anchored and centred panels (the script's header has the widescreen recipes for the easy park).

## See also

- [tpw-decomp](https://gitlab.com/tpw-decomp/tpw-decomp): the translation and its differential tests.
- [FIXES.md](FIXES.md): every place the port behaves differently from the Mac.

No license has been chosen yet.
