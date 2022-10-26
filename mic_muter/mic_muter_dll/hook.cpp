//////////////////////////////////////////////////////////////////////

#include "framework.h"

//////////////////////////////////////////////////////////////////////

HOOKDLL_API HHOOK mic_mute_hook;
HOOKDLL_API HWND main_hwnd;

//////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}
//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK mic_mute_hook_function(int nCode, WPARAM wParam, LPARAM lParam)
{
    LOG_CONTEXT("hook");

    auto const *data = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
    int const key = data->vkCode;
    DWORD const flags = data->flags;
    LOG_DEBUG("CODE: {}, wParam: {}, KEY: {}, FLAGS: {}", nCode, wParam, key, flags);
    if(nCode == HC_ACTION && wParam == WM_KEYDOWN && key == VK_F24) {
        PostMessage(main_hwnd, WM_HOTKEY_PRESSED, 0, 0);
        return 1;
    }
    return CallNextHookEx(mic_mute_hook, nCode, wParam, lParam);
}
