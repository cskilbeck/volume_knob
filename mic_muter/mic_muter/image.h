#pragma once

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    char const *get_overlay_svg(overlay_id id);

    struct image
    {
        LOG_CONTEXT("image");

        HDC dc{ nullptr };
        HGDIOBJ old_bmp{ nullptr };
        HBITMAP bmp{ nullptr };
        int width{ 0 };
        int height{ 0 };

        //////////////////////////////////////////////////////////////////////

        HRESULT create_solid(uint32 argb, int w, int h);
        HRESULT create_from_svg(char const *svg, int w, int h);
        void destroy();
    };
}