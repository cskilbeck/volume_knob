#pragma once

namespace chs::mic_muter
{
    struct settings
    {
        struct overlay_setting
        {
            // show it or don't
            bool enabled;

            // start fading out after this many seconds
            float fadeout_after_seconds;

            // take this many seconds to fade to final_opacity
            float fadeout_over_seconds;

            float final_opacity;
        };

        // 0 - 100 % where 0% is left/bottom-aligned, 100% is right/top aligned

        float overlay_position_x_percent;
        float overlay_position_y_percent;

        // 2 - 50 %
        float overlay_scale_percent;

        overlay_setting mute_overlay_setting;
        overlay_setting unmute_overlay_setting;
    };
}
