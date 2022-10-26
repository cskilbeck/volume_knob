//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <shellapi.h>
#include <Uxtheme.h>
#include <wrl/client.h>

//////////////////////////////////////////////////////////////////////

#include <vector>
#include <mutex>
#include <format>

//////////////////////////////////////////////////////////////////////

#include "../common_lib/util.h"
#include "../common_lib/ansi.h"
#include "../common_lib/logger.h"
#include "../mic_muter/mic_muter.h"

#include "hook.h"
