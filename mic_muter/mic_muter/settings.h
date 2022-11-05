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

        // need one of these for each state (muted, unmuted, disconnected)
        struct overlay_setting
        {
            // show it or don't
            bool enabled;

            // start fading out after this many seconds
            byte fadeout_time;

            // take this many seconds to fade to final_opacity
            byte fadeout_speed;

            // fade to this opacity
            byte fadeout_to;
        };

        overlay_setting overlay[num_overlay_ids]{
            { true, fadeout_after_5_seconds, fadeout_speed_slow, fadeout_to_50_percent },
            { true, fadeout_after_1_second, fadeout_speed_fast, fadeout_to_25_percent },
            { true, fadeout_after_0_seconds, fadeout_speed_medium, fadeout_to_invisible }
        };

        RECT overlay_position;

        bool run_at_startup{ true };

        byte hotkey{ VK_CAPITAL };
        byte modifiers{ keymod_ctrl };

        overlay_setting *get_overlay_setting(bool muted, bool attached);

        HRESULT save_run_at_startup();

        HRESULT save();
        HRESULT load();
    };

    extern settings_t settings;
}
