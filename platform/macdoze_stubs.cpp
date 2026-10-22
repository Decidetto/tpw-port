// macdoze_stubs.cpp: the Win32 window, message and GDI calls that macdoze compiles to `li r3,0; blr` (8 bytes each).
// The Mac shim never made a window or a device context of its own for these: the game gets its window from the
// Toolbox and the picture from OpenGL, so each answers 0 (NULL, FALSE or no error) and touches nothing. The calls the
// layer does give a meaning are in window.cpp, gdi.cpp and macdoze_misc.cpp.
#include "LbSCBack.h"
#include "WinScrn.h"
#include "platform/macdoze.h"

namespace NS_MacDoze {

void ClientToScreen(GrafPort*, tagPOINT*) {}   // 0x1000124c
int32_t DefWindowProc(GrafPort*, uint32_t, uint32_t, int32_t*) { return 0; }   // 0x100012e8
void DestroyWindow(GrafPort*) {}   // 0x10001330
int32_t GetSystemMetrics(int32_t) { return 0; }   // 0x100013bc
void* CreateWindowEx(uint32_t, const char*, const char*, uint32_t, int32_t, int32_t, int32_t, int32_t, GrafPort*, void*,
                     void*, void*) { return nullptr; }   // 0x100013f8
int32_t PeekMessage(tagMSG*, GrafPort*, uint32_t, uint32_t, uint32_t) { return 0; }   // 0x10001450
int32_t RegisterClass(const tagWNDCLASSA*) { return 0; }   // 0x100014ac
int32_t DispatchMessage(const tagMSG*) { return 0; }   // 0x10001504
int32_t TranslateMessage(const tagMSG*) { return 0; }   // 0x10001554
int32_t GetSystemPaletteEntries(InternalHandle*, uint32_t, uint32_t, TbPaletteEntry*) { return 0; }   // 0x100092e0
void* LoadIcon(void*, const char*) { return nullptr; }   // 0x10009364
void* GetStockObject(int32_t) { return nullptr; }   // 0x1000939c
InternalHandle* CreateDC(const char*, const char*, const char*, const _devicemodeA*) { return nullptr; }   // 0x100093d8
int32_t GetDeviceCaps(InternalHandle*, int32_t) { return 0; }   // 0x10009434
int32_t SystemParametersInfoA(uint32_t, uint32_t, void*, uint32_t) { return 0; }   // 0x1000d678

}  // namespace NS_MacDoze
