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
        enum fadeout_after : byte
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

        static constexpr char const *fadeout_after_names[]{
            "Straight away",
            "After 1 second",
            "After 5 seconds",
            "After 10 seconds",
            "Never"
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
            "25%",
            "50%"
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

        // need one of these for each state
        struct overlay_setting
        {
            // show it or don't
            bool enabled{ true };

            // start fading out after this many seconds
            byte fadeout_time{ fadeout_after_5_seconds };

            // take this many seconds to fade to final_opacity
            byte fadeout_speed{ fadeout_speed_medium };

            byte fadeout_to{ fadeout_to_25_percent };
        };

        overlay_setting overlay[num_overlay_ids];

        RECT overlay_position;

        bool run_at_startup{ true };

        overlay_setting *get_overlay_setting(bool muted, bool attached);

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

        HRESULT load()
        {
            LOG_CONTEXT("settings.load");

            SETTINGS_LOAD_VALUE(run_at_startup);
            SETTINGS_LOAD_VALUE(overlay_position);

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
    };

    extern settings_t settings;
}
