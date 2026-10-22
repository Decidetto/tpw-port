# tpw-port

A native port of Sim Theme Park (AKA Theme Park World), built on the [tpw-decomp](https://gitlab.com/tpw-decomp/tpw-decomp) translation of the
1999 Mac build. It runs on macOS and uses OpenGL (part of macOS).

tpw-decomp is the reference: the game's own functions, each behaving exactly as on the Mac, bugs included. This repository
adds what it takes to play:

- launcher/: the program (window, options, the game's start-up sequence) and the generated stand-ins for the names the
  code calls that nothing defines yet;
- platform/: the platform layer, the Mac Toolbox, Sound Sprockets, QuickTime and the game's Windows-style shims done on
  SDL3 (built from source in third_party/SDL3), OpenGL and CoreText;
- overrides/: the port's fixes, 53 so far, each one listed in [FIXES.md](FIXES.md) with what the Mac does, what the port does
  and why. Each file there is built in place of the tpw-decomp file of the same path; the rest of tpw-decomp is built as
  it is. The headers are read from a copy of tpw-decomp's headers in the build folder with the overridden ones laid over
  it, so every file of both repositories sees the port's version of a header.

## Where it stands

Every function of the game is translated (tpw-decomp is at 100% of functions), and the port plays from the lobby into a
park in both editions. Not everything is connected yet: a few hundred names the translated code calls (mostly per-file
call shims, the online library and some platform calls) have no definition in the port. The launcher stands in for each
one and logs "not wired up yet (does nothing): <name>" the first time it is reached; a platform call logs
"unimplemented platform call: <name>". The online features (the lobby's globe) do not work yet.

## Nothing from the game

Neither repository holds any part of the game. You need your own copy. The US edition needs only the Mac CD zip: the
launcher reads its data tables from the CD's "SimTheme Park" program and the game reads the CD's "Theme Park Data" folder
at run time. The European edition needs the PC image and the Mac CD zip as well: the data tables come from the Mac
program, and the PC disc cannot supply them. Not affiliated with or endorsed by Electronic Arts. Sim Theme Park and
Theme Park World are trademarks of their respective owners. No game content is included; you need your own discs.

## Game data

The port runs from the original downloads, unchanged. devtools/import_originals.py finds them by their MD5 (any file
names), unpacks them and checks every file it writes against the SHA-256 list it carries:

| Original | MD5 |
| --- | --- |
| From the US Mac CD (SimThemePark Mac US CD v2.0.1): the zip holding the CD image "Theme Park.toast"; all the US edition needs | f03abce3f375bc72d863cc527749e645 |
| From the demo and bonus item (ThemeParkWorld Mac Demo and Bonus Stuff): the Bonus Stuff zip; optional, its fonts serve only the platform tests | 1adeba0b0cf5edcc623d44181a321f28 |
| From the UK PC CD item (ThemeParkWorld PC UK-European CD Bullfrog EA): Redump 66484, ThemeParkWorld.img; optional, for the European (Theme Park World) edition, with the Mac CD zip (devtools/import_eur.py writes extracted/tpw-eur) | cd1cc4c33c1bd10673b855edd4b9e0f2 |
| From the same item: Redump 9465, TPWORLD.bin; optional, the other European image (either one will do; the two older ISO images, df7d864394ad46860984be60f2a3e448 and 50ffee97f9abc37e751f36d7239caea4, are still accepted) | 3c6590e438b3134e9208b407cc970d33 |

**Patches are not needed.** The classic updates (Mac 2.04b3, 2.1, 2.1.1, 2.1.4; PC v1.1, v2.0) only replaced the game
program. The port replaces that program with code translated from Mac 2.04b3, so their fixes are already in it and it
reads only data files. A patch file given to the import is ignored: the importer says so and carries on. Only the 2000
PC pressings are accepted, Redump 66484 and 9465. The 1999 pressing, Redump 66100, holds v1.0 data that the PC v2.0
patch updated, so the importer refuses it. It lacks files the European edition needs (711 of 845 confirmed); use 66484
or 9465.

The importer needs Python 3.8 or later and nothing else. Put the downloads in one folder, then from WORK, the folder
holding tpw-port:

    python3 tpw-port/devtools/import_originals.py --from DOWNLOADS --to extracted

It takes about half a minute and 2 GB of memory, writes nothing outside extracted/ and ends with "CHECK OK" (or the list
of the files that differ). `--check extracted` checks the folders again later; `--whole-disc` also writes the rest of the
CD (manuals, installers, extras), which nothing reads. The result:

    WORK/
      tpw-port/                        this repository (tpw-port/tpw-decomp is the submodule)
      extracted/
        disc-hfs/Theme Park Data/      each file's data fork, with NAME.rsrc (its resource fork) and NAME.finfo
        tpw-eur/Theme Park Data/       the European edition's game folder, when the PC image is there
        Theme_Park_World_Bonus_Stuff/  the bonus fonts, when that zip is there

The simplest way to play is to run build/tpw_launcher with no options. Its window asks for your discs: drop the US Mac
CD (or its zip or image) on it, plus the PC CD for the European edition. Every file is checked against the SHA-256 list
and copied into the launcher's data folder (on macOS ~/Library/Application Support/STP-TPW Launcher), which also holds
the saves. Press Play. `tpw_launcher --play us` (or eur) starts an edition from that folder without the window. The extracted
folders above still work: from WORK, `build/tpw_launcher --game "extracted/disc-hfs/Theme Park Data"` (or
devtools/run_launcher.sh, below).

## Building

Needs CMake 3.20, a C++17 compiler and Python 3 (OpenGL is part of macOS). SDL3, zlib and a trimmed FFmpeg are built
from source with the port (third_party/SDL3, third_party/zlib, third_party/ffmpeg); there is nothing to install
(-DTPW_MOVIES=OFF builds without the films).

    git clone --recurse-submodules <tpw-port>
    cmake -S tpw-port -B build
    cmake --build build -j8 --target tpw_launcher

The launcher reads the game's data tables at run time from the game folder's "SimTheme Park" program (or the one
--program names); the build holds none of them and needs no program. -DTPW_TABLES_AT_CONFIGURE=ON compiles them in at
configure time instead, from -DTPW_GAME_PROGRAM=<path> (default: the CD's program).

The submodule's URL is relative (../tpw-decomp), so it resolves to tpw-decomp beside tpw-port, on the same server or in
the same folder. A clone of a local folder needs `git -c protocol.file.allow=always submodule update --init`.

## Playing

    devtools/run_launcher.sh build NAME SECONDS [options]

runs build/tpw_launcher from the working folder with its own user folder (build/user-NAME) and log (build/launch-NAME.log)
and quits it after SECONDS. Options: --mute, --no-movies, --windowed, --display builtin, --background, --launcher-window, --play us|eur and
--data DIR (start from the launcher's data folder instead of extracted/), and the scripted
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

No license has been chosen yet for the port's own code. third_party/ffmpeg is FFmpeg 9.0.2 (trimmed), under the GNU LGPL
version 2.1 or later (third_party/ffmpeg/COPYING.LGPLv2.1); it is built from that source with the game, so you can change
it and rebuild. Details in third_party/ffmpeg/NOTE.md.

The port also builds SDL3 3.2.30 and zlib 1.3.2 (zlib License) and the IJG's libjpeg 6b (IJG License). The launcher's
text uses Fredoka and Nunito (SIL Open Font License 1.1, third_party/fonts) and is drawn with stb_truetype and
stb_rect_pack (MIT or public domain, third_party/stb). The launcher's About box lists the versions it was built with.
