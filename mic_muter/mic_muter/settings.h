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
            "Straight away",
            "After 1 second",
            "After 5 seconds",
            "After 10 seconds",
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

            char const *name{ nullptr };

            overlay_setting(char const *n) : name(n)
            {
            }
        };

        RECT overlay_position;

        enum overlay_page : int
        {
            page_muted = 0,
            page_unmuted = 1,
            page_disconnected = 2,

            num_overlay_pages = 3
        };

        overlay_setting mute_overlay{ "Muted" };
        overlay_setting unmute_overlay{ "Unmuted" };
        overlay_setting disconnected_overlay{ "Disconnected" };

        static overlay_setting *overlay_settings[num_overlay_pages];

        static int get_overlay_index(bool muted, bool attached);
        static overlay_setting *get_overlay_setting(bool muted, bool attached);

        bool run_at_startup{ true };

        HRESULT save_run_at_startup();

        //////////////////////////////////////////////////////////////////////

#define SETTINGS_SAVE_VALUE(x) HR(chs::util::registry_write("SOFTWARE\\MicMuter", #x, x))
#define SETTINGS_LOAD_VALUE(x) HR(chs::util::registry_read("SOFTWARE\\MicMuter", #x, &x))

        //////////////////////////////////////////////////////////////////////

        HRESULT save()
        {
            LOG_CONTEXT("settings.save");

            RECT rc;
            GetClientRect(overlay_hwnd, &rc);
            MapWindowPoints(overlay_hwnd, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);
            overlay_position = rc;

            SETTINGS_SAVE_VALUE(run_at_startup);
            SETTINGS_SAVE_VALUE(overlay_position);

            SETTINGS_SAVE_VALUE(mute_overlay.enabled);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_to_percent);

            SETTINGS_SAVE_VALUE(unmute_overlay.enabled);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_to_percent);

            SETTINGS_SAVE_VALUE(disconnected_overlay.enabled);
            SETTINGS_SAVE_VALUE(disconnected_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(disconnected_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(disconnected_overlay.fadeout_to_percent);

            HR(save_run_at_startup());

            return S_OK;
        }

        //////////////////////////////////////////////////////////////////////

        HRESULT load()
        {
            LOG_CONTEXT("settings.load");

            SETTINGS_LOAD_VALUE(run_at_startup);
            SETTINGS_LOAD_VALUE(overlay_position);

            SETTINGS_LOAD_VALUE(mute_overlay.enabled);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_to_percent);

            SETTINGS_LOAD_VALUE(unmute_overlay.enabled);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_to_percent);

            SETTINGS_LOAD_VALUE(disconnected_overlay.enabled);
            SETTINGS_LOAD_VALUE(disconnected_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(disconnected_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(disconnected_overlay.fadeout_to_percent);

            return S_OK;
        }
    };

    extern settings_t settings;
}
