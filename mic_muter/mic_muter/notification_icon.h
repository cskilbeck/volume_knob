#pragma once

namespace chs::mic_muter
{
    struct notification_icon
    {
        HRESULT load();
        HRESULT update(bool attached, bool muted);
        HRESULT destroy();

        HICON icon[num_overlay_ids];
    };
}
