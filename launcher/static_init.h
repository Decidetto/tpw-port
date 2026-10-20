// static_init.h: the main program's static initialisers (launcher/static_init.cpp).
#pragma once

// Runs the translated ones in the Mac's order, before the game's main.
void RunStaticInitialisers();
// The one running now, or nullptr (for the crash report).
const char* RunningStaticInitialiser();
