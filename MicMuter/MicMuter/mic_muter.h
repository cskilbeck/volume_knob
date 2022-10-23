#pragma once

#include "resource.h"

#define WM_VOLUMECHANGE (WM_USER + 12)
#define WM_ENDPOINTCHANGE (WM_USER + 13)
#define WM_NOTIFICATION_ICON (WM_USER + 14)

namespace chs
{
    extern HWND main_hwnd;
};