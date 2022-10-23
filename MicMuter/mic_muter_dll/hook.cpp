#include "framework.h"

HOOKDLL_API HHOOK mic_mute_hook;

namespace
{
    LOG_CONTEXT("hook");
}

HOOKDLL_API LRESULT CALLBACK mic_mute_hook_function(int nCode, WPARAM wParam, LPARAM lParam)
{
    auto const *data = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
    int const key = data->vkCode;
    DWORD const flags = data->flags;
    LOG_DEBUG("CODE: {}, wParam: {}, KEY: {}, FLAGS: {}", nCode, wParam, key, flags);
    if(nCode == HC_ACTION) {
        if(wParam == WM_KEYDOWN && key == 'C' && GetAsyncKeyState(VK_LCONTROL) < 0) {
            // double elapsed = timer.GetElapsed();
            // timer.Reset();
            // if(elapsed < MouseDoubleClickTime && key == lastKey) {
            //    PostMessage(hMainWindow, WMU_LAUNCH_BROWSER, key, 0);
            //    return 1;
            //}
            // lastKey = key;
        } else if(wParam == WM_KEYDOWN && key == 'V' && GetAsyncKeyState(VK_SHIFT) < 0) {
            // PostMessage(hMainWindow, WMU_PASTE_PLAIN_TEXT, key, 0);
            // return 1;
        }
    }
    return CallNextHookEx(mic_mute_hook, nCode, wParam, lParam);
}