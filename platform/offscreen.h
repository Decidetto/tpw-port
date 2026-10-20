// offscreen.h: the offscreen framebuffers the game draws into when the window is fitted to the screen (platform/fit.h).
// OpenGL calls only; the decisions (sizes, where the picture goes) are made by platform/window.cpp.
#pragma once
#include "fit.h"

namespace tpw::offscreen {

// Makes the game's framebuffer (colour and a 24-bit depth buffer, width x height) and the "present" framebuffer (colour
// only, the same size; the cursor is drawn into a copy of the game's picture so that the game's own never carries it),
// binds the game's, and sets its viewport. Needs the context current. False (nothing left behind) when the driver
// lacks GL_EXT_framebuffer_object / GL_EXT_framebuffer_blit or the framebuffer is not complete.
bool Create(int width, int height);
void Destroy();
bool Ready();

// Binds the framebuffer the game draws into for both drawing and reading, or the present one.
void BindGame();
void BindPresent();
void BindDefault();   // the window's framebuffer (what SDL_GL_SwapWindow needs bound)
// True between BindGame and the next Bind of another framebuffer: the game's own glDrawBuffer(GL_BACK) and
// glReadBuffer(GL_BACK) calls (which name the window's buffer) are then answered from the framebuffer's one buffer.
bool GameBound();

// Copies the game's picture into the present framebuffer, pixel for pixel. The game's framebuffer is bound afterwards.
void CopyGameToPresent();
// Binds the window's own framebuffer, clears it to black and scales the present framebuffer into the box (window pixels,
// from the top left), with linear filtering. The present framebuffer is bound afterwards.
void ScaleToWindow(const fit::PixelBox& box, int windowWide, int windowHigh);

// Empties OpenGL's error queue so that nothing this file did is blamed on the game's next glGetError (the game
// treats any error as a failure); logs the first one seen.
void DrainErrors(const char* where);

}  // namespace tpw::offscreen
