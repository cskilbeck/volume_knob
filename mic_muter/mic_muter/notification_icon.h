#pragma once

namespace chs
{
    struct notification_icon
    {
        HRESULT load(bool is_muted);
        HRESULT update(bool is_muted);
        HRESULT destroy();

        HICON muted_icon;
        HICON normal_icon;
        HICON missing_icon;
    };
}
