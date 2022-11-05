#pragma once

//////////////////////////////////////////////////////////////////////

#ifdef MICMUTERDLL_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif

//////////////////////////////////////////////////////////////////////

extern HOOKDLL_API HWND overlay_hwnd;
extern HOOKDLL_API HWND options_dlg;

extern HOOKDLL_API HHOOK hotkey_hook;
extern HOOKDLL_API byte hotkey_modifiers;
extern HOOKDLL_API byte hotkey_keycode;
extern HOOKDLL_API bool hotkey_scanning;

// track modifier key states manually...

static int constexpr keymod_ctrl = 1 << 0;
static int constexpr keymod_alt = 1 << 1;
static int constexpr keymod_shift = 1 << 2;
static int constexpr keymod_winkey = 1 << 3;

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK mic_mute_hook_function(int nCode, WPARAM wParam, LPARAM lParam);
