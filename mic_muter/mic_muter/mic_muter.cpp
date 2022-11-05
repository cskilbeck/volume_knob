//////////////////////////////////////////////////////////////////////

#include "framework.h"

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    static char const *overlay_names[mic_muter::num_overlay_ids] = { "Muted", "Unmuted", "Disconnected" };

    int get_overlay_id(bool muted, bool attached)
    {
        int index = overlay_id_disconnected;
        if(attached) {
            index = overlay_id_unmuted;
            if(muted) {
                index = overlay_id_muted;
            }
        }
        return index;
    }

    //////////////////////////////////////////////////////////////////////

    char const *get_overlay_name(int id)
    {
        id = std::clamp(id, 0, max_overlay_id);
        return overlay_names[id];
    }
}

//////////////////////////////////////////////////////////////////////

namespace
{
    LOG_CONTEXT("mic_muter");

    using namespace chs;

    using mic_muter::audio_controller;
    using mic_muter::image;
    using mic_muter::notification_icon;
    using mic_muter::overlay_id;
    using mic_muter::settings_t;

    using mic_muter::settings;

    int constexpr drag_idle_stop_seconds = 3;

    UINT_PTR constexpr TIMER_ID_WAIT = 101;
    UINT_PTR constexpr TIMER_ID_FADE = 102;
    UINT_PTR constexpr TIMER_ID_DRAG = 103;

    uint32 constexpr drag_window_flags = WS_POPUP;
    uint32 constexpr drag_window_ex_flags = WS_EX_TOPMOST;

    uint32 constexpr overlay_window_flags = WS_POPUP;
    uint32 constexpr overlay_window_ex_flags = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;

    char constexpr drag_window_class_name[] = "mic_muter_drag_EB5E918E-9B22-40D4-AD8A-6991DD92D360";
    char constexpr overlay_window_class_name[] = "mic_muter_overlay_FC29A1DC-16DE-4E9A-83E5-2DD9A5E034AA";

    //////////////////////////////////////////////////////////////////////

    HWND drag_hwnd;

    int overlay_size;

    ComPtr<audio_controller> audio;

    notification_icon notify_icon;

    bool mic_muted;
    bool mic_attached;

    image overlay_image[mic_muter::num_overlay_ids];

    uint64_t fade_ticks;

    uint32_t WM_TASKBARCREATED;

    HFONT menu_banner_font;

    //////////////////////////////////////////////////////////////////////

    image *get_current_image()
    {
        return &overlay_image[mic_muter::get_overlay_id(mic_muted, mic_attached)];
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT reload_images()
    {
        for(int i = 0; i < mic_muter::num_overlay_ids; ++i) {
            auto id = static_cast<overlay_id>(i);
            HR(overlay_image[i].create_from_svg(get_overlay_svg(id), overlay_size, overlay_size));
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT create_menu_banner_font()
    {
        NONCLIENTMETRICS ncm{ sizeof(NONCLIENTMETRICS) };
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, false);

        ncm.lfMenuFont.lfWidth = 0;
        ncm.lfMenuFont.lfHeight = (ncm.lfMenuFont.lfHeight * 125) / 100;
        ncm.lfMenuFont.lfWeight = FW_BOLD;

        menu_banner_font = CreateFontIndirect(&ncm.lfMenuFont);
        if(menu_banner_font == nullptr) {
            return WIN32_LAST_ERROR("CreateFont");
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT show_context_menu(HWND hwnd, POINT const &pt)
    {
        HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MENU_POPUP));
        if(hMenu == nullptr) {
            return WIN32_LAST_ERROR("LoadMenu");
        }
        DEFER(DestroyMenu(hMenu));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if(hSubMenu == nullptr) {
            return WIN32_LAST_ERROR("GetSubMenu");
        }

        MENUITEMINFO mi{ sizeof(MENUITEMINFO) };
        mi.fMask = MIIM_STATE | MIIM_STRING;
        mi.fState = mic_attached ? MFS_ENABLED : MFS_DISABLED;
        mi.dwTypeData = const_cast<LPSTR>(mic_muted ? "Unmute" : "Mute");
        if(!SetMenuItemInfo(hSubMenu, ID_POPUP_TOGGLE_MUTE, false, &mi)) {
            return WIN32_LAST_ERROR("SetMenuItemInfo");
        }

        // Banner menuitem
        mi.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID;
        mi.fType = MFT_OWNERDRAW;
        mi.wID = ID_POPUP_MICMUTER;
        mi.dwItemData = reinterpret_cast<ULONG_PTR>(mic_muter::app_name);
        mi.cch = static_cast<UINT>(strlen(mic_muter::app_name));
        if(!SetMenuItemInfo(hSubMenu, ID_POPUP_MICMUTER, false, &mi)) {
            return WIN32_LAST_ERROR("SetMenuItemInfo(2)");
        }

        UINT uFlags = TPM_RIGHTBUTTON;
        if(GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
            uFlags |= TPM_RIGHTALIGN;
        } else {
            uFlags |= TPM_LEFTALIGN;
        }

        SetForegroundWindow(hwnd);

        if(!TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL)) {
            return WIN32_LAST_ERROR("TrackPopupMenuEx");
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    void update_layered_window(int alpha)
    {
        BLENDFUNCTION bf{};
        bf.BlendFlags = 0;
        bf.BlendOp = AC_SRC_OVER;
        bf.SourceConstantAlpha = static_cast<BYTE>(alpha);
        bf.AlphaFormat = AC_SRC_ALPHA;

        image *img = get_current_image();
        RECT rc;
        GetWindowRect(overlay_hwnd, &rc);

        SIZE sz{ overlay_size, overlay_size };
        POINT dst{ rc.left, rc.top };
        POINT src{ 0, 0 };

        HDC screen_dc = GetDC(nullptr);
        DEFER(ReleaseDC(nullptr, screen_dc));

        if(!UpdateLayeredWindow(overlay_hwnd, screen_dc, &dst, &sz, img->dc, &src, RGB(0, 0, 0), &bf, ULW_ALPHA)) {
            WIN32_LAST_ERROR("UpdateLayeredWindow");
        }
    }

    //////////////////////////////////////////////////////////////////////

    void draw_image(HWND hWnd, HDC hdc)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC hdcMem;
        HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rc, BPBF_COMPATIBLEBITMAP, nullptr, &hdcMem);
        if(hBufferedPaint != nullptr) {
            hdc = hdcMem;
        }

        image *img = get_current_image();
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, w, h, img->dc, 0, 0, img->width, img->height, SRCCOPY);

        if(hBufferedPaint != nullptr) {
            EndBufferedPaint(hBufferedPaint, true);
        }
    }

    //////////////////////////////////////////////////////////////////////

    void do_fadeout()
    {
        KillTimer(overlay_hwnd, TIMER_ID_WAIT);
        KillTimer(overlay_hwnd, TIMER_ID_FADE);

        int overlay_id = mic_muter::get_overlay_id(mic_muted, mic_attached);
        settings_t::overlay_setting const &overlay_setting = settings.overlay[overlay_id];

        if(!overlay_setting.enabled) {
            ShowWindow(overlay_hwnd, SW_HIDE);
            return;
        }

        update_layered_window(255);
        ShowWindow(overlay_hwnd, SW_SHOW);
        BringWindowToTop(overlay_hwnd);

        int fade_after = settings_t::fadeout_after_ms[overlay_setting.fadeout_time];

        if(fade_after > 0) {
            SetTimer(overlay_hwnd, TIMER_ID_WAIT, fade_after, nullptr);
        } else if(fade_after == 0) {
            fade_ticks = GetTickCount64();
            SetTimer(overlay_hwnd, TIMER_ID_FADE, 16, nullptr);
        }
    }

    //////////////////////////////////////////////////////////////////////
    // Make the overlay window draggable/sizeable

    void start_move_overlay()
    {
        KillTimer(overlay_hwnd, TIMER_ID_FADE);
        KillTimer(overlay_hwnd, TIMER_ID_WAIT);

        RECT client_rect;
        GetClientRect(overlay_hwnd, &client_rect);

        AdjustWindowRectEx(&client_rect, drag_window_flags, false, drag_window_ex_flags);
        int width = client_rect.right - client_rect.left;
        int height = client_rect.bottom - client_rect.top;
        ClientToScreen(overlay_hwnd, (LPPOINT)(&client_rect.left));

        ShowWindow(overlay_hwnd, SW_HIDE);

        SetWindowPos(drag_hwnd, nullptr, client_rect.left, client_rect.top, width, height, SWP_SHOWWINDOW);
        InvalidateRect(drag_hwnd, nullptr, true);
        SetActiveWindow(drag_hwnd);
        SetForegroundWindow(drag_hwnd);
        SetTimer(overlay_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
    }

    //////////////////////////////////////////////////////////////////////
    // finished dragging/sizing the overlay window, make it overlayish again

    void stop_move_overlay()
    {
        KillTimer(overlay_hwnd, TIMER_ID_DRAG);

        ShowWindow(drag_hwnd, SW_HIDE);

        // get final size
        RECT client_rect;
        GetClientRect(drag_hwnd, &client_rect);
        overlay_size = client_rect.right - client_rect.left;

        // make new correctly sized images
        reload_images();

        // new window position
        RECT window_rect;
        GetWindowRect(drag_hwnd, &window_rect);
        SetWindowPos(overlay_hwnd, nullptr, window_rect.left, window_rect.top, overlay_size, overlay_size,
                     SWP_SHOWWINDOW);

        do_fadeout();
    }

    //////////////////////////////////////////////////////////////////////

    void handle_menu_option(int const wmId)
    {
        switch(wmId) {

        case ID_POPUP_QUIT:
            PostMessage(overlay_hwnd, WM_APP_QUIT_PLEASE, 0, 0);
            break;

        case ID_POPUP_SHOW_OPTIONS:
            mic_muter::show_options_dialog(mic_muted, mic_attached);
            break;

        case ID_POPUP_TOGGLE_MUTE:
            audio->toggle_mute();
            break;

        case ID_POPUP_MOVE_OVERLAY:
            start_move_overlay();
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK drag_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {

        case WM_GETMINMAXINFO: {

            HMONITOR hMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hMonitor, &mi);

            int max_size = std::min(mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top) * 80 / 100;

            int min_size = GetSystemMetrics(SM_CXSIZEFRAME) * 8;

            MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
            mmi->ptMinTrackSize = { min_size, min_size };
            mmi->ptMaxTrackSize = { max_size, max_size };
            break;
        }

        case WM_CREATE:
            drag_hwnd = hWnd;
            break;

        case WM_MOVING:
            SetTimer(overlay_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
            break;

        case WM_SIZING: {

            // force it to be square
            RECT *r = reinterpret_cast<RECT *>(lParam);

            int w = r->right - r->left;
            int h = r->bottom - r->top;

            switch(wParam) {

            case WMSZ_RIGHT:
            case WMSZ_BOTTOMLEFT:
            case WMSZ_BOTTOMRIGHT:
                r->bottom = r->top + w;
                break;

            case WMSZ_LEFT:
            case WMSZ_TOPLEFT:
            case WMSZ_TOPRIGHT:
                r->top = r->bottom - w;
                break;

            case WMSZ_TOP:
                r->left = r->right - h;
                break;

            case WMSZ_BOTTOM:
                r->right = r->left + h;
                break;
            }
            SetTimer(overlay_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
            return 1;
        }

        // resizing without a window border
        case WM_NCHITTEST: {
            LRESULT const hits[] = { HTCAPTION, HTLEFT,        HTRIGHT,       HTLEFT,
                                     HTTOP,     HTTOPLEFT,     HTTOPRIGHT,    HTTOPLEFT,
                                     HTBOTTOM,  HTBOTTOMLEFT,  HTBOTTOMRIGHT, HTBOTTOMLEFT,
                                     HTTOP,     HTBOTTOMRIGHT, HTBOTTOMRIGHT, HTBOTTOMRIGHT };
            POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &p);
            RECT rc;
            GetClientRect(hWnd, &rc);
            int const width = rc.right - rc.left;
            int const height = rc.bottom - rc.top;
            int const x_border = GetSystemMetrics(SM_CXSIZEFRAME) * 2;
            int const y_border = GetSystemMetrics(SM_CYSIZEFRAME) * 2;
            int const left = std::abs(p.x) < x_border;
            int const right = std::abs(p.x - width) < x_border;
            int const top = std::abs(p.y) < y_border;
            int const bottom = std::abs(p.y - height) < y_border;
            return hits[left | (right << 1) | (top << 2) | (bottom << 3)];
        }

        case WM_ACTIVATE:
            if(wParam == WA_INACTIVE) {
                stop_move_overlay();
            }
            break;

        case WM_ERASEBKGND:
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            draw_image(hWnd, hdc);
            EndPaint(hWnd, &ps);
        } break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK overlay_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {

        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
            mmi->ptMinTrackSize.x = GetSystemMetrics(SM_CXSIZEFRAME) * 8;
            mmi->ptMinTrackSize.y = GetSystemMetrics(SM_CYSIZEFRAME) * 8;
            break;
        }

        case WM_CREATE: {

            RECT rc;

            if(FAILED(settings.load())) {
                HMONITOR hMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
                MONITORINFO mi = { sizeof(mi) };
                GetMonitorInfo(hMonitor, &mi);
                overlay_size = (mi.rcMonitor.right - mi.rcMonitor.left) * 10 / 100;
                POINT const pt = { (mi.rcMonitor.right - overlay_size) / 2, (mi.rcMonitor.bottom - overlay_size) / 2 };
                rc = { pt.x, pt.y, pt.x + overlay_size, pt.y + overlay_size };
            } else {
                rc = settings.overlay_position;
                overlay_size = rc.right - rc.left;
                hotkey_modifiers = settings.modifiers;
                hotkey_keycode = settings.hotkey;
            }

            SetWindowPos(hWnd, HWND_TOPMOST, rc.left, rc.top, overlay_size, overlay_size, 0);

            overlay_hwnd = hWnd;

            settings.save();

            audio.Attach(new(std::nothrow) audio_controller());

            hotkey_hook = SetWindowsHookEx(WH_KEYBOARD_LL, mic_mute_hook_function, GetModuleHandle(nullptr), 0);

            audio->init();

            audio->get_mic_info(&mic_attached, &mic_muted);

            notify_icon.load();
            notify_icon.update(mic_attached, mic_muted);

            reload_images();

            create_menu_banner_font();

            PostMessage(hWnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;
        }

        case WM_DESTROY:
            DestroyWindow(drag_hwnd);
            drag_hwnd = nullptr;
            notify_icon.destroy();
            UnhookWindowsHookEx(hotkey_hook);
            hotkey_hook = nullptr;
            for(auto &img : overlay_image) {
                img.destroy();
            }
            BufferedPaintUnInit();
            audio->Dispose();
            audio.Reset();
            settings.save();
            PostQuitMessage(0);
            break;

        case WM_COMMAND:
            handle_menu_option(LOWORD(wParam));
            break;

        case WM_TIMER: {
            switch(wParam) {

            case TIMER_ID_DRAG:
                KillTimer(overlay_hwnd, TIMER_ID_DRAG);
                stop_move_overlay();
                break;

            case TIMER_ID_WAIT:
                KillTimer(hWnd, TIMER_ID_WAIT);
                fade_ticks = GetTickCount64();
                SetTimer(hWnd, TIMER_ID_FADE, 16, nullptr);
                break;

            case TIMER_ID_FADE: {
                auto *s = &settings.overlay[mic_muter::get_overlay_id(mic_muted, mic_attached)];
                uint64 now = GetTickCount64();
                float elapsed = static_cast<float>(now - fade_ticks);
                float duration = static_cast<float>(settings_t::fadeout_over_ms[s->fadeout_speed]);
                int target_alpha = settings_t::fadeout_to_alpha[s->fadeout_to];
                int alpha_range = 255 - target_alpha;
                float d = std::min(1.0f, elapsed / duration);
                int window_alpha = 255 - static_cast<int>(d * alpha_range);
                if(window_alpha > 1) {
                    update_layered_window(window_alpha);
                } else {
                    ShowWindow(hWnd, SW_HIDE);
                }
                if(elapsed >= duration) {
                    KillTimer(hWnd, TIMER_ID_FADE);
                }
            } break;
            }
            break;
        }

        case WM_APP_NOTIFICATION_ICON:
            switch(LOWORD(lParam)) {
            case NIN_SELECT:
            case WM_CONTEXTMENU:
                show_context_menu(overlay_hwnd, { LOWORD(wParam), HIWORD(wParam) });
                break;
            }
            break;

        case WM_APP_SHOW_OVERLAY: {
            audio->get_mic_info(&mic_attached, &mic_muted);
            notify_icon.update(mic_attached, mic_muted);
            do_fadeout();
            break;
        }

        case WM_APP_ENDPOINT_CHANGE:
            audio->change_endpoint();
            PostMessage(overlay_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;

        case WM_APP_HOTKEY_PRESSED:
            audio->toggle_mute();
            break;

        case WM_APP_QUIT_PLEASE:
            DestroyWindow(overlay_hwnd);
            break;

        case WM_MEASUREITEM: {
            auto mi = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
            if(mi->itemID == ID_POPUP_MICMUTER) {
                HDC dc = GetDC(nullptr);
                HFONT old_font = SelectFont(dc, menu_banner_font);
                char const *txt = reinterpret_cast<char const *>(mi->itemData);
                int len = static_cast<int>(strlen(txt));
                DWORD tx = GetTabbedTextExtent(dc, txt, len, 0, nullptr);
                SelectFont(dc, old_font);
                ReleaseDC(nullptr, dc);
                mi->itemWidth = LOWORD(tx);
                mi->itemHeight = HIWORD(tx);
                return TRUE;
            }
        } break;

        case WM_DRAWITEM: {
            auto di = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
            if(di->itemID == ID_POPUP_MICMUTER) {
                char const *txt = reinterpret_cast<char const *>(di->itemData);
                int len = static_cast<int>(strlen(txt));
                HFONT old_font = SelectFont(di->hDC, menu_banner_font);
                DWORD tx = GetTabbedTextExtent(di->hDC, txt, len, 0, nullptr);
                int w = LOWORD(tx);
                int x = (di->rcItem.right - w) / 2;
                ExtTextOut(di->hDC, di->rcItem.left + x, di->rcItem.top, 0, &di->rcItem, txt, len, nullptr);
                SelectFont(di->hDC, old_font);
            }
        } break;

        default:
            if(message == WM_TASKBARCREATED) {
                LOG_INFO("WM_TASKBARCREATED!");
                notify_icon.destroy();
                notify_icon.load();
                notify_icon.update(mic_attached, mic_muted);
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT init_window()
    {
        HR(BufferedPaintInit());

        WNDCLASSEX wcex = { sizeof(wcex) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = overlay_wnd_proc;
        wcex.hInstance = GetModuleHandle(nullptr);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = GetStockBrush(NULL_BRUSH);
        wcex.lpszClassName = overlay_window_class_name;

        if(RegisterClassEx(&wcex) == 0) {
            return WIN32_LAST_ERROR("RegisterClassEx(overlay)");
        }

        wcex.lpfnWndProc = drag_wnd_proc;
        wcex.lpszClassName = drag_window_class_name;

        if(RegisterClassEx(&wcex) == 0) {
            return WIN32_LAST_ERROR("RegisterClassEx(drag)");
        }

        CreateWindowEx(overlay_window_ex_flags, overlay_window_class_name, mic_muter::app_name, overlay_window_flags, 0,
                       0, 0, 0, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

        if(overlay_hwnd == nullptr) {
            return WIN32_LAST_ERROR("CreateWindowEx(overlay)");
        }

        CreateWindowEx(drag_window_ex_flags, drag_window_class_name, mic_muter::app_name, drag_window_flags, 0, 0, 0, 0,
                       nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

        if(drag_hwnd == nullptr) {
            return WIN32_LAST_ERROR("CreateWindowEx(drag)");
        }

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT win_main(HINSTANCE hInstance)
    {
        // single instance admin

        HANDLE single_instance_mutex = CreateMutex(nullptr, true, "micmutermutex82C56D8B-E69C-4DA1-BC5A-39B27188E00D");
        if(single_instance_mutex == nullptr) {
            return WIN32_LAST_ERROR("CreateMutex");
        }
        DEFER(CloseHandle(single_instance_mutex));

        if(GetLastError() == ERROR_ALREADY_EXISTS) {
            HWND hwnd = FindWindow(drag_window_class_name, mic_muter::app_name);
            if(hwnd != nullptr) {
                PostMessage(hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            }
            return S_OK;
        }

        HR(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
        DEFER(CoUninitialize());

        SetProcessDPIAware();

        WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

        HR(init_window());

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return S_OK;
    }
}

//////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    LOG_CONTEXT("wWinMain");

    HRESULT hr = win_main(hInstance);
    if(FAILED(hr)) {
        LOG_ERROR("win_main() failed: {}", chs::util::windows_error_text(hr));
    }
    return 0;
}
