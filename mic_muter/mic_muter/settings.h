//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    struct settings_t
    {
        LOG_CONTEXT("settings");

        // clang-format off

        // after mute status is changed, wait this long before fading out
        enum fadeout_time : byte
        {
            fadeout_after_0_seconds = 0,
            fadeout_after_1_second = 1,
            fadeout_after_5_seconds = 2,
            fadeout_after_10_seconds = 3,
            fadeout_never = 4
        };

        static constexpr int fadeout_after_ms[]{
            0,
            1000,
            5000,
            10000,
            -1
        };

        static constexpr char const *fadeout_after_ms_names[]{
            "straight away",
            "after 1 second",
            "after 5 seconds",
            "after 10 seconds",
            "never"
        };

        // fade out to this alpha
        enum fadeout_to : byte
        {
            fadeout_to_invisible = 0,
            fadeout_to_25_percent = 1,
            fadeout_to_50_percent = 2
        };

        static constexpr int fadeout_to_alpha[]{
            0,
            96,
            150
        };

        static constexpr char const *fadeout_to_names[]{
            "Invisible",
            "Ghostly",
            "Transparent"
        };

        // fade out this quickly
        enum fadeout_speed : byte
        {
            fadeout_speed_slow = 0,
            fadeout_speed_medium = 1,
            fadeout_speed_fast = 2,
            fadeout_speed_instantly = 2,
        };
        static constexpr int fadeout_over_ms[]{
            3000,
            1000,
            250,
            1
        };

        static constexpr char const *fadeout_speed_names[]{
            "Slow",
            "Medium",
            "Fast",
            "Instant"
        };
        // clang-format on

        // need one each of these for mute/unmute

        struct overlay_setting
        {
            // show it or don't
            bool enabled{ true };

            // start fading out after this many seconds
            byte fadeout_time_ms{ fadeout_after_5_seconds };

            // take this many seconds to fade to final_opacity
            byte fadeout_speed_ms{ fadeout_speed_medium };

            byte fadeout_to_percent{ fadeout_to_25_percent };
        };

        RECT overlay_position;

        overlay_setting mute_overlay;
        overlay_setting unmute_overlay;

        bool run_at_startup{ true };

        //////////////////////////////////////////////////////////////////////

#define SETTINGS_SAVE_VALUE(x) HR(chs::util::registry_write("SOFTWARE\\MicMuter", #x, x))
#define SETTINGS_LOAD_VALUE(x) HR(chs::util::registry_read("SOFTWARE\\MicMuter", #x, &x))

        //////////////////////////////////////////////////////////////////////

        HRESULT save()
        {
            LOG_CONTEXT("settings.save");

            RECT rc;
            GetClientRect(main_hwnd, &rc);
            MapWindowPoints(main_hwnd, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);
            overlay_position = rc;

            SETTINGS_SAVE_VALUE(run_at_startup);
            SETTINGS_SAVE_VALUE(mute_overlay.enabled);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_to_percent);
            SETTINGS_SAVE_VALUE(unmute_overlay.enabled);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_to_percent);
            SETTINGS_SAVE_VALUE(overlay_position);

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

        //////////////////////////////////////////////////////////////////////

        HRESULT load()
        {
            LOG_CONTEXT("settings.load");

            SETTINGS_LOAD_VALUE(run_at_startup);
            SETTINGS_LOAD_VALUE(mute_overlay.enabled);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_to_percent);
            SETTINGS_LOAD_VALUE(unmute_overlay.enabled);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_to_percent);
            SETTINGS_LOAD_VALUE(overlay_position);
            return S_OK;
        }
    };

    extern settings_t settings;
}
