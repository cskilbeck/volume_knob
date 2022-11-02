#include "framework.h"

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    settings_t settings;

    // clang-format off
    settings_t::overlay_setting *settings_t::overlay_settings[num_overlay_ids] =
    {
        &settings.mute_overlay,
        &settings.unmute_overlay,
        &settings.disconnected_overlay
    };
    // clang-format on

    //////////////////////////////////////////////////////////////////////

    settings_t::overlay_setting *settings_t::get_overlay_setting(bool muted, bool attached)
    {
        return overlay_settings[get_overlay_id(muted, attached)];
    }

    HRESULT settings_t::save_run_at_startup()
    {
#if !defined(_DEBUG)
        HKEY key;
        char const *run_key = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
        LSTATUS status = RegCreateKey(HKEY_CURRENT_USER, run_key, &key);
        if(status != ERROR_SUCCESS) {
            return WIN32_ERROR(status, "RegCreateKey");
        }
        DEFER(RegCloseKey(key));

        if(run_at_startup) {
            char filename[MAX_PATH * 2];
            DWORD len = GetModuleFileName(GetModuleHandle(nullptr), filename, _countof(filename));
            if(len == 0 || len == _countof(filename)) {
                return WIN32_LAST_ERROR("GetModuleFileName");
            }
            status = RegSetValueEx(key, "MicMuter", 0, REG_SZ, reinterpret_cast<BYTE const *>(filename), len);
            if(status != ERROR_SUCCESS) {
                return WIN32_ERROR(status, "RegSetValueEx");
            }
        } else {
            status = RegDeleteValue(key, "MicMuter");
            if(status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
                return WIN32_ERROR(status, "RegDeleteValue");
            }
        }
#endif
        return S_OK;
    }
};
