//////////////////////////////////////////////////////////////////////

#include "framework.h"

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Msimg32.lib")

#pragma comment(linker,                                                                                         \
                "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'" \
                " processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    LOG_CONTEXT("mic_muter");

    char const *app_name = "MicMuter";

    DWORD constexpr RGB_BLACK = RGB(0, 0, 0);

    // int constexpr drag_idle_stop_seconds = 2;
    int constexpr drag_idle_stop_seconds = 3;

    UINT_PTR constexpr TIMER_ID_WAIT = 101;
    UINT_PTR constexpr TIMER_ID_FADE = 102;
    UINT_PTR constexpr TIMER_ID_DRAG = 103;

    uint32 constexpr drag_window_flags = WS_POPUP;
    uint32 constexpr drag_window_ex_flags = WS_EX_TOPMOST;

    uint32 constexpr overlay_window_flags = WS_POPUP;
    uint32 constexpr overlay_window_ex_flags = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;

    constexpr char drag_window_class_name[] = "mic_muter_drag_EB5E918E-9B22-40D4-AD8A-6991DD92D360";
    constexpr char overlay_window_class_name[] = "mic_muter_overlay_FC29A1DC-16DE-4E9A-83E5-2DD9A5E034AA";

    constexpr char window_title[] = "MicMuter";

    //////////////////////////////////////////////////////////////////////

    HINSTANCE hInst;

    HWND drag_hwnd;

    bool double_buffered;

    int overlay_size;

    ComPtr<audio_controller> audio;

    notification_icon notify_icon;

    bool mic_muted;
    bool mic_attached;

    image overlay_image[num_overlay_ids];

    uint64 fade_ticks;

    uint32 WM_TASKBARCREATED;

    HFONT menu_banner_font;

    //////////////////////////////////////////////////////////////////////

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

    image *get_current_image()
    {
        return &overlay_image[get_overlay_id(mic_muted, mic_attached)];
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT reload_images()
    {
        for(int i = 0; i < num_overlay_ids; ++i) {
            auto id = static_cast<overlay_id>(i);
            HR(overlay_image[i].create_from_svg(get_svg(id), overlay_size, overlay_size));
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT show_context_menu(HWND hwnd, POINT const &pt)
    {
        bool present{ true };
        bool muted{ false };
        HR(audio->get_mic_info(&present, &muted));
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
        mi.fState = present ? MFS_ENABLED : MFS_DISABLED;
        mi.dwTypeData = const_cast<LPSTR>(muted ? "Unmute" : "Mute");
        if(!SetMenuItemInfo(hSubMenu, ID_POPUP_MUTE, false, &mi)) {
            return WIN32_LAST_ERROR("SetMenuItemInfo");
        }

        // Banner menuitem
        mi.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID;
        mi.fType = MFT_OWNERDRAW;
        mi.wID = ID_POPUP_MICMUTER;
        mi.dwItemData = reinterpret_cast<ULONG_PTR>(app_name);
        mi.cch = static_cast<UINT>(strlen(app_name));
        if(!SetMenuItemInfo(hSubMenu, ID_POPUP_MICMUTER, false, &mi)) {
            return WIN32_LAST_ERROR("SetMenuItemInfo(2)");
        }

        SetForegroundWindow(hwnd);
        UINT uFlags = TPM_RIGHTBUTTON;
        if(GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
            uFlags |= TPM_RIGHTALIGN;
        } else {
            uFlags |= TPM_LEFTALIGN;
        }

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
        bf.SourceConstantAlpha = alpha;
        bf.AlphaFormat = AC_SRC_ALPHA;

        image *img = get_current_image();
        RECT rc;
        GetWindowRect(overlay_hwnd, &rc);

        SIZE sz{ overlay_size, overlay_size };
        POINT pt_dst{ rc.left, rc.top };
        POINT pt_src{ 0, 0 };

        HDC screen_dc = GetDC(nullptr);
        DEFER(ReleaseDC(nullptr, screen_dc));

        if(!UpdateLayeredWindow(overlay_hwnd, screen_dc, &pt_dst, &sz, img->dc, &pt_src, RGB_BLACK, &bf, ULW_ALPHA)) {
            WIN32_LAST_ERROR("UpdateLayeredWindow");
        }
    }

    //////////////////////////////////////////////////////////////////////

    void draw_image(HWND hWnd, HDC hdc)
    {
        LOG_DEBUG("PAINT!");

        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HPAINTBUFFER hBufferedPaint = nullptr;
        if(double_buffered) {
            HDC hdcMem;
            hBufferedPaint = BeginBufferedPaint(hdc, &rc, BPBF_COMPATIBLEBITMAP, nullptr, &hdcMem);
            if(hBufferedPaint != nullptr) {
                hdc = hdcMem;
            }
        }

        image *img = get_current_image();
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, w, h, img->dc, 0, 0, overlay_size, overlay_size, SRCCOPY);

        if(hBufferedPaint) {
            BufferedPaintMakeOpaque(hBufferedPaint, nullptr);
            EndBufferedPaint(hBufferedPaint, true);
        }
    }

    //////////////////////////////////////////////////////////////////////

    void do_fadeout()
    {
        settings_t::overlay_setting *s = settings_t::get_overlay_setting(mic_muted, mic_attached);
        LOG_INFO("do_fadeout({})", s->name);
        if(!s->enabled) {
            ShowWindow(overlay_hwnd, SW_HIDE);
            return;
        }
        update_layered_window(255);
        ShowWindow(overlay_hwnd, SW_SHOW);
        BringWindowToTop(overlay_hwnd);
        int fade_after = settings_t::fadeout_after_ms[s->fadeout_time];
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
        LOG_DEBUG("start_move_overlay");

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

        // get window caption/border offset
        AdjustWindowRectEx(&client_rect, drag_window_flags, false, drag_window_ex_flags);
        int x_offset = -client_rect.left;
        int y_offset = -client_rect.top;

        reload_images();

        // new window position
        RECT window_rect;
        GetWindowRect(drag_hwnd, &window_rect);
        window_rect.left += x_offset;
        window_rect.top += y_offset;

        // new window style

        // new window position
        SetWindowPos(overlay_hwnd, nullptr, window_rect.left, window_rect.top, overlay_size, overlay_size,
                     SWP_SHOWWINDOW);

        do_fadeout();
    }

    //////////////////////////////////////////////////////////////////////

    bool is_overlay_position_mode()
    {
        LONG f = GetWindowLong(overlay_hwnd, GWL_EXSTYLE);
        return f == drag_window_ex_flags;
    }

    //////////////////////////////////////////////////////////////////////

    void handle_menu_option(int const wmId)
    {
        switch(wmId) {

        case ID_POPUP_QUIT:
            PostMessage(overlay_hwnd, WM_APP_QUIT_PLEASE, 0, 0);
            // CloseWindow(main_hwnd);
            break;

        case ID_POPUP_ABOUTMICMUTER:
            show_options_dialog(mic_muted, mic_attached);
            break;

        case ID_POPUP_MUTE:
            audio->toggle_mute();
            break;

        case ID_OVERLAY_POSITION:
            start_move_overlay();
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK drag_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {

        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
            mmi->ptMinTrackSize.x = GetSystemMetrics(SM_CXSIZEFRAME) * 8;
            mmi->ptMinTrackSize.y = GetSystemMetrics(SM_CYSIZEFRAME) * 8;
            break;
        }

        case WM_CREATE:
            drag_hwnd = hWnd;
            break;

        case WM_MOVING:
            SetTimer(overlay_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
            break;

        case WM_SIZING: {
            RECT *r = reinterpret_cast<RECT *>(lParam);

            util::UnadjustWindowRectEx(r, drag_window_flags, false, drag_window_ex_flags);

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
            AdjustWindowRectEx(r, drag_window_flags, false, drag_window_ex_flags);
            InvalidateRect(hWnd, nullptr, false);
            SetTimer(overlay_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
            return 1;
        }

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
            int const x_border = GetSystemMetrics(SM_CXSIZEFRAME) * 2;    // double because internal only
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
            if(!is_overlay_position_mode()) {
                draw_image(hWnd, hdc);
            }
            EndPaint(hWnd, &ps);
        } break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT create_menu_banner_font()
    {
        NONCLIENTMETRICS ncm{ sizeof(NONCLIENTMETRICS) };
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, false);

        ncm.lfMenuFont.lfWidth = 0;
        ncm.lfMenuFont.lfHeight *= 125;
        ncm.lfMenuFont.lfHeight /= 100;
        ncm.lfMenuFont.lfWeight = FW_BOLD;

        menu_banner_font = CreateFontIndirect(&ncm.lfMenuFont);

        if(menu_banner_font == nullptr) {
            return WIN32_LAST_ERROR("CreateFont");
        }
        return S_OK;
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
            }

            SetWindowPos(hWnd, HWND_TOPMOST, rc.left, rc.top, overlay_size, overlay_size, 0);

            overlay_hwnd = hWnd;

            settings.save();

            audio.Attach(new(std::nothrow) audio_controller());

            mic_mute_hook = SetWindowsHookEx(WH_KEYBOARD_LL, mic_mute_hook_function, hInst, 0);

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
            UnhookWindowsHookEx(mic_mute_hook);
            mic_mute_hook = nullptr;
            for(auto &img : overlay_image) {
                img.destroy();
            }
            if(double_buffered) {
                BufferedPaintUnInit();
            }
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
                LOG_INFO("DRAG STOP!");
                KillTimer(overlay_hwnd, TIMER_ID_DRAG);
                stop_move_overlay();
                break;

            case TIMER_ID_WAIT:
                KillTimer(hWnd, TIMER_ID_WAIT);
                fade_ticks = GetTickCount64();
                SetTimer(hWnd, TIMER_ID_FADE, 16, nullptr);
                break;

            case TIMER_ID_FADE: {
                settings_t::overlay_setting *s = settings_t::get_overlay_setting(mic_muted, mic_attached);
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
            LOG_DEBUG("WM_APP_SHOW_OVERLAY");
            audio->get_mic_info(&mic_attached, &mic_muted);
            notify_icon.update(mic_attached, mic_muted);
            KillTimer(hWnd, TIMER_ID_WAIT);
            KillTimer(hWnd, TIMER_ID_FADE);
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
        double_buffered = SUCCEEDED(BufferedPaintInit());
        // double_buffered = FALSE;

        WNDCLASSEX wcex = { sizeof(wcex) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = overlay_wnd_proc;
        wcex.hInstance = hInst;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszClassName = overlay_window_class_name;

        if(RegisterClassEx(&wcex) == 0) {
            return WIN32_LAST_ERROR(GetLastError());
        }

        wcex.lpfnWndProc = drag_wnd_proc;
        wcex.lpszClassName = drag_window_class_name;

        if(RegisterClassEx(&wcex) == 0) {
            return WIN32_LAST_ERROR(GetLastError());
        }

        CreateWindowEx(overlay_window_ex_flags, overlay_window_class_name, window_title, overlay_window_flags, 0, 0, 0,
                       0, nullptr, nullptr, hInst, nullptr);

        if(overlay_hwnd == nullptr) {
            return WIN32_LAST_ERROR(GetLastError());
        }

        CreateWindowEx(drag_window_ex_flags, drag_window_class_name, window_title, drag_window_flags, 0, 0, 0, 0,
                       nullptr, nullptr, hInst, nullptr);

        if(drag_hwnd == nullptr) {
            return WIN32_LAST_ERROR(GetLastError());
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
            HWND hwnd = FindWindow(drag_window_class_name, window_title);
            if(hwnd != nullptr) {
                PostMessage(hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            }
            return S_OK;
        }

        // WM_TIMER exception supression (

        BOOL suppress = true;
        SetUserObjectInformationW(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &suppress,
                                  sizeof(suppress));

        // Load the settings

        hInst = hInstance;

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

    HRESULT hr = chs::mic_muter::win_main(hInstance);
    if(FAILED(hr)) {
        LOG_ERROR("win_main() failed: {}", chs::util::windows_error_text(hr));
    }
    return 0;
}
