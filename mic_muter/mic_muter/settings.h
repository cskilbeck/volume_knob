//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    struct settings_t
    {
        LOG_CONTEXT("settings");

        // after mute status is changed, wait this long before fading out
        enum fadeout_time : byte
        {
            fadeout_after_1_second = 0,
            fadeout_after_5_seconds = 1,
            fadeout_after_10_seconds = 2,
            fadeout_never = 3
        };

        // fade out to this alpha
        enum fadeout_to : byte
        {
            fadeout_to_invisible = 0,
            fadeout_to_25_percent = 1,
            fadeout_to_50_percent = 2
        };

        // fade out this quickly
        enum fadeout_speed : byte
        {
            fadout_speed_slow = 0,
            fadout_speed_medium = 1,
            fadeout_speed_fast = 2,
            fadeout_speed_instantly = 2,
        };

        // corresponding values for the enums

        static constexpr int fadeout_after_ms[]{ 1000, 5000, 10000, 0 };

        static constexpr int fadeout_to_percent[]{ 0, 25, 50 };

        static constexpr int fadeout_over_ms[]{ 10000, 3000, 1000, 1 };

        static constexpr char const *fadeout_after_ms_names[]{ "after 1 second", "after 5 seconds", "after 10 seconds",
                                                               "Never" };

        static constexpr char const *fadeout_to_names[]{ "Invisible", "25 percent", "50 percent" };

        static constexpr char const *fadeout_speed_names[]{ "Slow", "Medium", "Fast", "Instant" };

        // need one each of these for mute/unmute

        struct overlay_setting
        {
            // show it or don't
            bool enabled{ true };

            // start fading out after this many seconds
            byte fadeout_time_ms{ fadeout_after_5_seconds };

            // take this many seconds to fade to final_opacity
            byte fadeout_speed_ms{ fadeout_speed_fast };

            byte fadeout_to_percent{ fadeout_to_25_percent };
        };

        RECT overlay_position;

        overlay_setting mute_overlay;
        overlay_setting unmute_overlay;

        bool run_at_startup{ true };

        //////////////////////////////////////////////////////////////////////

#define SETTINGS_SAVE_VALUE(x) HR(chs::util::registry_write(#x, x))
#define SETTINGS_LOAD_VALUE(x) HR(chs::util::registry_read(#x, &x))

        //////////////////////////////////////////////////////////////////////

        HRESULT save()
        {
            SETTINGS_SAVE_VALUE(run_at_startup);
            SETTINGS_SAVE_VALUE(mute_overlay.enabled);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(mute_overlay.fadeout_to_percent);
            SETTINGS_SAVE_VALUE(unmute_overlay.enabled);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_time_ms);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_speed_ms);
            SETTINGS_SAVE_VALUE(unmute_overlay.fadeout_to_percent);
            return S_OK;
        }

        //////////////////////////////////////////////////////////////////////

        HRESULT load()
        {
            SETTINGS_LOAD_VALUE(run_at_startup);
            SETTINGS_LOAD_VALUE(mute_overlay.enabled);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(mute_overlay.fadeout_to_percent);
            SETTINGS_LOAD_VALUE(unmute_overlay.enabled);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_time_ms);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_speed_ms);
            SETTINGS_LOAD_VALUE(unmute_overlay.fadeout_to_percent);
            return S_OK;
        }
    };
}
