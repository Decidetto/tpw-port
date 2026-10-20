// indent_dispatch.cpp: the three calls of UIMain_IndentClient (0x10180940) that unit v51039 declared as free functions so its tests
// could fake them (include/ui_main.h, namespace indent_callee) and that need symbols the test library does not compile. In the
// launcher they are the calls themselves: CoolRect's constructor on the storage, InterfaceWindow::GetPosition and
// InterfaceWindow::SetClientRect. (Adjust and RectDtor are defined with UIMain_IndentClient.)
#include <new>

#include "InterfaceWindow.h"
#include "UI_Base.h"
#include "ui_main.h"

namespace indent_callee {

void RectCtor(void* rect) { new (rect) CoolRect(); }

int32_t GetPosition(InterfaceWindow* window, void* rect, int16_t subtract) {
    return window->GetPosition(static_cast<CoolRect*>(rect), subtract);
}

void SetClientRect(InterfaceWindow* window, void* rect) { window->SetClientRect(static_cast<CoolRect*>(rect)); }

}  // namespace indent_callee
