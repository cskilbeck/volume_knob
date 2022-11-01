//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

#include "targetver.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <Uxtheme.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

//////////////////////////////////////////////////////////////////////

#include <string>
#include <mutex>
#include <algorithm>
#include <format>
#include <new>
#include <vector>

//////////////////////////////////////////////////////////////////////

#include "resource.h"

#include "mic_muter.h"
#include "../common_lib/ansi.h"
#include "../common_lib/logger.h"
#include "../common_lib/util.h"

#include "../mic_muter_dll/hook.h"

#include "image.h"
#include "audio_controller.h"
#include "notification_icon.h"
#include "settings.h"
#include "options_dlg.h"
