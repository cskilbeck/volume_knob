#include "framework.h"

//////////////////////////////////////////////////////////////////////

#define SETTINGS_SAVE_VALUE(x) HR(chs::util::registry_write("SOFTWARE\\MicMuter", #x, x))
#define SETTINGS_LOAD_VALUE(x) HR(chs::util::registry_read("SOFTWARE\\MicMuter", #x, &x))

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    settings_t settings;

    //////////////////////////////////////////////////////////////////////

    settings_t::overlay_setting *settings_t::get_overlay_setting(bool muted, bool attached)
    {
        return &overlay[get_overlay_id(muted, attached)];
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT settings_t::save()
    {
        RECT rc;
        GetClientRect(overlay_hwnd, &rc);
        MapWindowPoints(overlay_hwnd, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);
        overlay_position = rc;

        SETTINGS_SAVE_VALUE(run_at_startup);
        SETTINGS_SAVE_VALUE(overlay_position);
        SETTINGS_SAVE_VALUE(hotkey);
        SETTINGS_SAVE_VALUE(modifiers);

        SETTINGS_SAVE_VALUE(overlay[overlay_id_muted].enabled);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_muted].fadeout_time);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_muted].fadeout_speed);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_muted].fadeout_to);

        SETTINGS_SAVE_VALUE(overlay[overlay_id_unmuted].enabled);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_unmuted].fadeout_time);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_unmuted].fadeout_speed);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_unmuted].fadeout_to);

        SETTINGS_SAVE_VALUE(overlay[overlay_id_disconnected].enabled);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_disconnected].fadeout_time);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_disconnected].fadeout_speed);
        SETTINGS_SAVE_VALUE(overlay[overlay_id_disconnected].fadeout_to);

        HR(save_run_at_startup());

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT settings_t::load()
    {
        SETTINGS_LOAD_VALUE(run_at_startup);
        SETTINGS_LOAD_VALUE(overlay_position);
        SETTINGS_LOAD_VALUE(hotkey);
        SETTINGS_LOAD_VALUE(modifiers);

        SETTINGS_LOAD_VALUE(overlay[overlay_id_muted].enabled);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_muted].fadeout_time);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_muted].fadeout_speed);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_muted].fadeout_to);

        SETTINGS_LOAD_VALUE(overlay[overlay_id_unmuted].enabled);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_unmuted].fadeout_time);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_unmuted].fadeout_speed);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_unmuted].fadeout_to);

        SETTINGS_LOAD_VALUE(overlay[overlay_id_disconnected].enabled);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_disconnected].fadeout_time);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_disconnected].fadeout_speed);
        SETTINGS_LOAD_VALUE(overlay[overlay_id_disconnected].fadeout_to);

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

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
            status = RegSetValueEx(key, app_name, 0, REG_SZ, reinterpret_cast<BYTE const *>(filename), len);
            if(status != ERROR_SUCCESS) {
                return WIN32_ERROR(status, "RegSetValueEx");
            }
        } else {
            status = RegDeleteValue(key, app_name);
            if(status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
                return WIN32_ERROR(status, "RegDeleteValue");
            }
        }
#endif
        return S_OK;
    }
};
