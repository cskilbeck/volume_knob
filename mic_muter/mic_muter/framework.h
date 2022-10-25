// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"

// Windows Header Files
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <Uxtheme.h>
#include <wrl/client.h>
#include <wincodec.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <string>
#include <mutex>
#include <algorithm>
#include <format>
#include <new>
#include <vector>

#include "mic_muter.h"
#include "util.h"
#include "ansi.h"
#include "logger.h"
#include "audio_controller.h"
#include "notification_icon.h"

#include "../mic_muter_dll/hook.h"
