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
# A test run also opens behind the other windows and never takes the focus (--background), so the
# person working on that screen is not interrupted; pass --foreground to have it in front.
background="--background"
rest=""
for arg in "$@"; do
  if [ "$arg" = "--foreground" ]; then background=""; else rest="$rest $arg"; fi
done
mkdir -p "$build/user-$name"
# Sign the build with a stable identity (TPW_SIGN_IDENTITY, by default the keychain's "Apple Development"
# certificate). The linker's ad-hoc signature changes with every build, so macOS took each one for a new
# app and asked again for the builds' removable volume; a signed build keeps the grant. No identity: the
# ad-hoc signature stays.
codesign -f -s "${TPW_SIGN_IDENTITY:-Apple Development}" "$build/tpw_launcher" >/dev/null 2>&1 || true
cd "$root" || exit 1
"$build/tpw_launcher" --game "extracted/disc-hfs/Theme Park Data" --user "$build/user-$name" --windowed $display $background $rest > "$build/launch-$name.log" 2>&1 &
pid=$!
n=0
while kill -0 $pid 2>/dev/null; do
  sleep 1; n=$((n + 1))
  if [ $n -ge $limit ]; then kill -9 $pid; echo "force-quit after ${limit}s" >> "$build/launch-$name.log"; break; fi
done
wait $pid 2>/dev/null
echo "exit=$?" >> "$build/launch-$name.log"
