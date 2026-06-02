#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>

extern "C" void DisableWindowsIME() {
    ImmDisableIME(-1);
}
#endif
