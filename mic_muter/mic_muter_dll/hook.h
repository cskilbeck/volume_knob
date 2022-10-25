#pragma once

//////////////////////////////////////////////////////////////////////

#ifdef MICMUTERDLL_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif

//////////////////////////////////////////////////////////////////////

extern HOOKDLL_API HHOOK mic_mute_hook;
extern HOOKDLL_API HWND main_hwnd;

//////////////////////////////////////////////////////////////////////

HOOKDLL_API LRESULT CALLBACK mic_mute_hook_function(int nCode, WPARAM wParam, LPARAM lParam);
