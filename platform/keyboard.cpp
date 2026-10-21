// keyboard.cpp: SamsUtilities::UKeyboard, the modifier-key query object of Sam's Utilities (an import of the main
// application; 0xc bytes on the Mac with a vtable whose slots are +0x08 destructor, +0x0c IsShiftDown, +0x10 IsOptionDown,
// +0x14 IsControlDown, +0x18 IsCommandDown, +0x1c GetModifiers, +0x20 IsKeyDown, +0x24 and +0x28 Translate). The game makes
// one on first use (SOUND_Init's Keyboard() in src/game/Sound.cpp stores it in SamsUtilities::gKeyboard) and asks only
// IsOptionDown and IsCommandDown: the sound channels' safe mode is "Option and Command held at start-up". On the host the
// answers come from SDL's modifier state through the same mapping the window's key events use (tpw::MacModifiersFromSDL:
// command 0x100, option 0x800). The answers are the Mac's bytes: 1 held, 0 not.
#include <SDL3/SDL.h>

#include "TbAudioSystem.h"
#include "platform.h"

namespace SamsUtilities {

UKeyboard::UKeyboard() {}

static uint8_t ModifierHeld(uint16_t macBit) {
    return (tpw::MacModifiersFromSDL(static_cast<uint16_t>(SDL_GetModState())) & macBit) != 0 ? 1 : 0;
}

uint8_t UKeyboard::IsOptionDown() { return ModifierHeld(0x800); }

uint8_t UKeyboard::IsCommandDown() { return ModifierHeld(0x100); }

}  // namespace SamsUtilities
