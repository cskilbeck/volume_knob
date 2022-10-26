#pragma once

namespace chs::mic_muter
{
    struct notification_icon
    {
        HRESULT load();
        HRESULT update(bool attached, bool muted);
        HRESULT destroy();

        HICON muted_icon;
        HICON normal_icon;
        HICON missing_icon;
    };
}
