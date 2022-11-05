//////////////////////////////////////////////////////////////////////

#include "framework.h"

//////////////////////////////////////////////////////////////////////

HOOKDLL_API HHOOK hotkey_hook{ nullptr };
HOOKDLL_API HWND overlay_hwnd{ nullptr };
HOOKDLL_API HWND options_dlg{ nullptr };

// default hotkey is ctrl-capslock
HOOKDLL_API byte hotkey_modifiers = keymod_ctrl;
HOOKDLL_API byte hotkey_keycode = VK_CAPITAL;

HOOKDLL_API bool hotkey_scanning{ false };

//////////////////////////////////////////////////////////////////////

namespace
{
    // modifier key states are tracked manually because...

    byte modifiers{ 0 };
}

//////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK mic_mute_hook_function(int nCode, WPARAM wParam, LPARAM lParam)
{
    LOG_CONTEXT("hook");

    if(nCode == HC_ACTION) {

        auto kbd_hookstruct = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        byte const keycode = static_cast<byte>(kbd_hookstruct->vkCode);

        int const press = (wParam == WM_SYSKEYDOWN || wParam == WM_KEYDOWN) ? 0xff : 0;
        int const release = (wParam == WM_SYSKEYUP || wParam == WM_KEYUP) ? 0xff : 0;

        switch(keycode) {

        case VK_LCONTROL:
        case VK_RCONTROL:
            modifiers = (modifiers | (press & keymod_ctrl)) & ~(release & keymod_ctrl);
            break;

        case VK_LMENU:
        case VK_RMENU:
            modifiers = (modifiers | (press & keymod_alt)) & ~(release & keymod_alt);
            break;

        case VK_LSHIFT:
        case VK_RSHIFT:
            modifiers = (modifiers | (press & keymod_shift)) & ~(release & keymod_shift);
            break;

        case VK_LWIN:
        case VK_RWIN:
            modifiers = (modifiers | (press & keymod_winkey)) & ~(release & keymod_winkey);
            break;

        default:
            LOG_DEBUG("PRESS: {}, KEY: 0x{:02x}, MOD: {:08x}", press, keycode, modifiers);
            if(press) {
                if(hotkey_scanning) {
                    hotkey_scanning = false;
                    hotkey_modifiers = modifiers;
                    hotkey_keycode = keycode;
                    PostMessage(options_dlg, WM_APP_HOTKEY_PRESSED, keycode, modifiers);
                    return 1;
                } else if(keycode == hotkey_keycode && modifiers == hotkey_modifiers) {
                    PostMessage(overlay_hwnd, WM_APP_HOTKEY_PRESSED, keycode, modifiers);
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(hotkey_hook, nCode, wParam, lParam);
}
