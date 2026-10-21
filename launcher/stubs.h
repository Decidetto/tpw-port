// stubs.h: the interface between the generated stand-ins (stubs_generated.cpp, written by
// gen_stubs.py at build time) and the launcher.
#pragma once

// Called by a stand-in every time it runs: `name` is the function it stands in for,
// `platform` tells a platform call from a game function that is not wired up yet, and
// `first` is true the first time that stand-in is hit.
void tpw_stub_called(const char* name, bool platform, bool first);

// How many stand-ins the build has, and the ones hit so far (returns how many were hit;
// fills at most `capacity` names).
int tpw_stub_count();
int tpw_stub_hits(const char** names, int capacity);
