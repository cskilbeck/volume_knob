#pragma once

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    char const *get_svg(overlay_id id);

    struct image
    {
        LOG_CONTEXT("image");

        HDC dc{ nullptr };
        HGDIOBJ old_bmp{ nullptr };
        HBITMAP bmp{ nullptr };

        //////////////////////////////////////////////////////////////////////

        HRESULT create_solid(uint32 argb, int w, int h);
        HRESULT create_from_svg(char const *svg, int w, int h);
        void destroy();
    };
}