#pragma once

#define WM_APP_SHOW_OVERLAY (WM_USER + 12)
#define WM_APP_ENDPOINT_CHANGE (WM_USER + 13)
#define WM_APP_NOTIFICATION_ICON (WM_USER + 14)
#define WM_APP_HOTKEY_PRESSED (WM_USER + 15)
#define WM_APP_QUIT_PLEASE (WM_USER + 16)    // 0x0410

namespace chs::mic_muter
{
    static char constexpr app_name[] = "MicMuter";

    enum overlay_id : int
    {
        overlay_id_muted = 0,
        overlay_id_unmuted = 1,
        overlay_id_disconnected = 2,

        num_overlay_ids = 3
    };

    int constexpr max_overlay_id = overlay_id_disconnected;

    //////////////////////////////////////////////////////////////////////

    char const *get_overlay_name(int overlay_id);

    int get_overlay_id(bool muted, bool attached);
}