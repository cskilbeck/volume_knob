#pragma once

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    struct image
    {
        LOG_CONTEXT("image");

        HDC dc{ nullptr };
        HGDIOBJ old_bmp{ nullptr };
        HBITMAP bmp{ nullptr };

        //////////////////////////////////////////////////////////////////////

        HRESULT create_solid(uint32 argb, int w, int h)
        {
            uint32 stride = w * sizeof(uint32);
            uint32 img_size = h * stride;

            BITMAPV5HEADER bmi{};
            bmi.bV5Size = sizeof(BITMAPV5HEADER);
            bmi.bV5Width = w;
            bmi.bV5Height = -h;
            bmi.bV5Planes = 1;
            bmi.bV5BitCount = 32;
            bmi.bV5Compression = BI_BITFIELDS;
            bmi.bV5SizeImage = img_size;
            bmi.bV5RedMask = 0x00ff0000;
            bmi.bV5GreenMask = 0x0000ff00;
            bmi.bV5BlueMask = 0x000000ff;
            bmi.bV5AlphaMask = 0xff000000;
            bmi.bV5CSType = LCS_WINDOWS_COLOR_SPACE;
            bmi.bV5Intent = LCS_GM_GRAPHICS;

            BITMAPINFO src_bmi{};
            src_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            src_bmi.bmiHeader.biWidth = w;
            src_bmi.bmiHeader.biHeight = -h;
            src_bmi.bmiHeader.biPlanes = 1;
            src_bmi.bmiHeader.biBitCount = 32;
            src_bmi.bmiHeader.biCompression = BI_RGB;

            HDC dc = GetDC(nullptr);
            DEFER(ReleaseDC(nullptr, dc));

            uint32 *buffer = new uint32[img_size];

            for(uint32 i = 0; i < img_size; ++i) {
                buffer[i] = argb;
            }

            DEFER(delete[] buffer);

            bmp = CreateDIBitmap(dc, (BITMAPINFOHEADER const *)&bmi, CBM_INIT, buffer, &src_bmi, DIB_RGB_COLORS);
            if(bmp == nullptr) {
                return WIN32_LAST_ERROR("CreateDIBitmap");
            }
            dc = CreateCompatibleDC(nullptr);
            old_bmp = SelectObject(dc, bmp);
            return S_OK;
        }

        //////////////////////////////////////////////////////////////////////

        HRESULT create_from_svg(char const *svg, int w, int h)
        {
            destroy();

            HR(util::svg_to_bitmap(svg, w, h, &bmp));
            dc = CreateCompatibleDC(nullptr);
            old_bmp = SelectObject(dc, bmp);
            return S_OK;
        }

        //////////////////////////////////////////////////////////////////////

        void destroy()
        {
            if(old_bmp != nullptr && dc != nullptr) {
                SelectObject(dc, old_bmp);
                old_bmp = nullptr;
            }
            if(bmp != nullptr) {
                DeleteObject(bmp);
                bmp = nullptr;
            }
            if(dc != nullptr) {
                DeleteDC(dc);
                dc = nullptr;
            }
        }
    };
}