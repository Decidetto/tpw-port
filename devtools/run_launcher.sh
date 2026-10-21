#!/bin/sh
# run_launcher.sh BUILD_DIR NAME SECONDS [launcher options]: runs BUILD_DIR/tpw_launcher from
# the project root with its own user folder (BUILD_DIR/user-NAME) and log
# (BUILD_DIR/launch-NAME.log), and force-quits it after SECONDS. (The launcher turns a plain
# kill into a quit event, which a hung loop never sees, so the kill is -9.) Unless the options
# name a --display, the window opens on the MacBook's built-in screen: test and development
# windows must never land on the maintainer's external monitor (2026-10-13).
build=$1; name=$2; limit=$3; shift 3
root=$(cd "$(dirname "$0")/../.." && pwd)
display="--display builtin"
for arg in "$@"; do [ "$arg" = "--display" ] && display=""; done
# The launcher is widescreen (16:9) at its largest preset by default, but a test run is 4:3 at
# 2048 x 1536 unless the options name an --aspect or a --resolution: the click recipes in HANDOFF.md
# and the process logs are 4:3 game pixels (2048 x 1536), and their frames stay comparable with the
# older ones. A widescreen run passes --aspect 16:9 and
# moves the clicks on right- and centre-anchored panels; the easy-park recipe at 16:9 (2730 x 1536) is
#   --aspect 16:9 --click 2008,121@200 --type Tess@230 --click 1759,566@300 --click 143,1177@800
#   --click 261,1414@2300
# (the lobby's "Create New Player" bar is right-anchored, 682 pixels further right; the name
# dialog is centred, 341 further right; the park buttons bottom left do not move). At 16:10 (2458 x 1536)
# the two moved clicks are 1736,121 and 1623,566. The launcher's own default (16:9 at 3840 x 2160, the
# 2730-pixel recipe times 1.40625; --launcher-defaults, below, runs it with neither option) is
#   --launcher-defaults --click 2824,170@200 --type Tess@230 --click 2474,796@300
#   --click 201,1655@800 --click 367,1988@2300
aspect="--aspect 4:3"
for arg in "$@"; do [ "$arg" = "--aspect" ] && aspect=""; done
resolution="--resolution 2048x1536"
for arg in "$@"; do [ "$arg" = "--resolution" ] && resolution=""; done
# --launcher-defaults (the script's own option) passes neither, for a run exactly as a player starts the game.
for arg in "$@"; do [ "$arg" = "--launcher-defaults" ] && aspect="" && resolution=""; done
# A test run also opens behind the other windows and never takes the focus (--background), so the
# person working on that screen is not interrupted; pass --foreground to have it in front.
background="--background"
rest=""
for arg in "$@"; do
  if [ "$arg" = "--foreground" ]; then background=""; elif [ "$arg" != "--launcher-defaults" ]; then rest="$rest $arg"; fi
done
mkdir -p "$build/user-$name"
# Sign the build with a stable identity (TPW_SIGN_IDENTITY, by default the keychain's "Apple Development"
# certificate). The linker's ad-hoc signature changes with every build, so macOS took each one for a new
# app and asked again for the builds' removable volume; a signed build keeps the grant. No identity: the
# ad-hoc signature stays.
codesign -f -s "${TPW_SIGN_IDENTITY:-Apple Development}" "$build/tpw_launcher" >/dev/null 2>&1 || true
cd "$root" || exit 1
"$build/tpw_launcher" --game "extracted/disc-hfs/Theme Park Data" --user "$build/user-$name" --windowed $display $aspect $resolution $background $rest > "$build/launch-$name.log" 2>&1 &
pid=$!
n=0
while kill -0 $pid 2>/dev/null; do
  sleep 1; n=$((n + 1))
  if [ $n -ge $limit ]; then kill -9 $pid; echo "force-quit after ${limit}s" >> "$build/launch-$name.log"; break; fi
done
wait $pid 2>/dev/null
echo "exit=$?" >> "$build/launch-$name.log"
