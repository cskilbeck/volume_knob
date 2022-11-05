//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define _WIN32_WINNT _WIN32_WINNT_WIN7

#include <windows.h>
#include <wrl/client.h>

//////////////////////////////////////////////////////////////////////

#include <vector>
#include <mutex>
#include <format>

//////////////////////////////////////////////////////////////////////

#pragma warning(disable : 4100)

#include "../common_lib/ansi.h"
#include "../common_lib/logger.h"
#include "../common_lib/util.h"
#include "../mic_muter/mic_muter.h"

#include "hook.h"
