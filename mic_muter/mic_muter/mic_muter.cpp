//////////////////////////////////////////////////////////////////////

#include "framework.h"

#include "images/microphone_normal_svg.h"
#include "images/microphone_mute_svg.h"

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

    //////////////////////////////////////////////////////////////////////

    HINSTANCE hInst;

    DWORD constexpr RGB_BLACK = RGB(0, 0, 0);

    ComPtr<audio_controller> audio;

    notification_icon notify_icon;

    settings_t settings;

    bool mic_muted;
    bool mic_attached;

    bool double_buffered = false;

    image muted_image;
    image normal_image;

    int img_size;

    uint64 ticks;

    HWND drag_hwnd{ nullptr };

    HWND options_dlg{ nullptr };

    // int constexpr drag_idle_stop_seconds = 2;
    int constexpr drag_idle_stop_seconds = 100;

    UINT_PTR constexpr TIMER_ID_WAIT = 101;
    UINT_PTR constexpr TIMER_ID_FADE = 102;
    UINT_PTR constexpr TIMER_ID_DRAG = 103;

    uint32 constexpr drag_window_flags = WS_POPUP;
    uint32 constexpr drag_window_ex_flags = WS_EX_TOPMOST;

    uint32 constexpr overlay_window_flags = WS_POPUP;
    uint32 constexpr overlay_window_ex_flags = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;

    constexpr char drag_window_class_name[] = "mic_muter_overlay_FC29A1DC-16DE-4E9A-83E5-2DD9A5E034AA";
    constexpr char overlay_window_class_name[] = "mic_muter_drag_EB5E918E-9B22-40D4-AD8A-6991DD92D360";

    constexpr char window_title[] = "MicMuter";

    uint32 WM_TASKBARCREATED;

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

        image *img = mic_muted ? &muted_image : &normal_image;

        RECT rc;
        GetWindowRect(main_hwnd, &rc);

        SIZE sz{ img_size, img_size };
        POINT pt_dst{ rc.left, rc.top };
        POINT pt_src{ 0, 0 };

        HDC screen_dc = GetDC(nullptr);
        DEFER(ReleaseDC(nullptr, screen_dc));

        if(!UpdateLayeredWindow(main_hwnd, screen_dc, &pt_dst, &sz, img->dc, &pt_src, RGB_BLACK, &bf, ULW_ALPHA)) {
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

        image *img = mic_muted ? &muted_image : &normal_image;
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, w, h, img->dc, 0, 0, img_size, img_size, SRCCOPY);

        if(hBufferedPaint) {
            BufferedPaintMakeOpaque(hBufferedPaint, nullptr);
            EndBufferedPaint(hBufferedPaint, true);
        }
    }

    //////////////////////////////////////////////////////////////////////

    void do_fadeout()
    {
        auto &s = mic_muted ? settings.mute_overlay : settings.unmute_overlay;
        if(mic_attached && s.enabled) {
            update_layered_window(255);
            ShowWindow(main_hwnd, SW_SHOW);
            int fade_after = settings_t::fadeout_after_ms[s.fadeout_time_ms];
            if(fade_after > 0) {
                SetTimer(main_hwnd, TIMER_ID_WAIT, fade_after, nullptr);
            } else if(fade_after == 0) {
                ticks = GetTickCount64();
                SetTimer(main_hwnd, TIMER_ID_FADE, 16, nullptr);
            } else {
                // -1 == fadeout never
            }
        } else {
            ShowWindow(main_hwnd, SW_HIDE);
        }
    }

    //////////////////////////////////////////////////////////////////////
    // Make the overlay window draggable/sizeable

    void start_move_overlay()
    {
        LOG_DEBUG("start_move_overlay");

        KillTimer(main_hwnd, TIMER_ID_FADE);
        KillTimer(main_hwnd, TIMER_ID_WAIT);

        RECT client_rect;
        GetClientRect(main_hwnd, &client_rect);

        AdjustWindowRectEx(&client_rect, drag_window_flags, false, drag_window_ex_flags);
        int width = client_rect.right - client_rect.left;
        int height = client_rect.bottom - client_rect.top;
        ClientToScreen(main_hwnd, (LPPOINT)(&client_rect.left));

        ShowWindow(main_hwnd, SW_HIDE);

        SetWindowPos(drag_hwnd, nullptr, client_rect.left, client_rect.top, width, height, SWP_SHOWWINDOW);
        InvalidateRect(drag_hwnd, nullptr, true);
        SetActiveWindow(drag_hwnd);
        SetForegroundWindow(drag_hwnd);
        SetTimer(main_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
    }

    //////////////////////////////////////////////////////////////////////
    // finished dragging/sizing the overlay window, make it overlayish again

    void stop_move_overlay()
    {
        KillTimer(main_hwnd, TIMER_ID_DRAG);

        ShowWindow(drag_hwnd, SW_HIDE);

        // get final size
        RECT client_rect;
        GetClientRect(drag_hwnd, &client_rect);
        img_size = client_rect.right - client_rect.left;

        // get window caption/border offset
        AdjustWindowRectEx(&client_rect, drag_window_flags, false, drag_window_ex_flags);
        int x_offset = -client_rect.left;
        int y_offset = -client_rect.top;

        muted_image.create_from_svg(microphone_mute_svg, img_size, img_size);
        normal_image.create_from_svg(microphone_normal_svg, img_size, img_size);

        // new window position
        RECT window_rect;
        GetWindowRect(drag_hwnd, &window_rect);
        window_rect.left += x_offset;
        window_rect.top += y_offset;

        // new window style

        // new window position
        SetWindowPos(main_hwnd, nullptr, window_rect.left, window_rect.top, img_size, img_size, SWP_SHOWWINDOW);

        do_fadeout();
    }

    //////////////////////////////////////////////////////////////////////

    bool is_overlay_position_mode()
    {
        LONG f = GetWindowLong(main_hwnd, GWL_EXSTYLE);
        return f == drag_window_ex_flags;
    }

    //////////////////////////////////////////////////////////////////////

    void handle_menu_option(int const wmId)
    {
        switch(wmId) {

        case ID_POPUP_QUIT:
            DestroyWindow(main_hwnd);
            break;

        case ID_POPUP_ABOUTMICMUTER:
            if(options_dlg != nullptr) {
                SetForegroundWindow(options_dlg);
                SetActiveWindow(options_dlg);
            } else {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), main_hwnd, options_dlg_proc);
            }
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
            SetTimer(main_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
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
            SetTimer(main_hwnd, TIMER_ID_DRAG, drag_idle_stop_seconds * 1000, nullptr);
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

            // Make BLACK the transparency color and use 25% alpha
            // SetLayeredWindowAttributes(hWnd, RGB_BLACK, 0, LWA_ALPHA | LWA_COLORKEY);

            HMONITOR hMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hMonitor, &mi);

            img_size = (mi.rcMonitor.right - mi.rcMonitor.left) * 5 / 100;

            POINT const pt = { mi.rcMonitor.right - img_size * 2, mi.rcMonitor.bottom - img_size * 2 };

            RECT rc{ pt.x, pt.y, pt.x + img_size, pt.y + img_size };

            if(settings.overlay_position.left != settings.overlay_position.right) {
                rc = settings.overlay_position;
                img_size = rc.right - rc.left;
            }

            SetWindowPos(hWnd, HWND_TOPMOST, rc.left, rc.top, img_size, img_size, 0);

            main_hwnd = hWnd;

            audio.Attach(new(std::nothrow) audio_controller());

            mic_mute_hook = SetWindowsHookEx(WH_KEYBOARD_LL, mic_mute_hook_function, hInst, 0);

            audio->init();

            audio->get_mic_info(&mic_attached, &mic_muted);

            notify_icon.load();
            notify_icon.update(mic_attached, mic_muted);

            // check here if they want the overlay to be shown based on mute status
            PostMessage(hWnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;
        }

        case WM_DESTROY:
            notify_icon.destroy();
            UnhookWindowsHookEx(mic_mute_hook);
            muted_image.destroy();
            normal_image.destroy();
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
                KillTimer(main_hwnd, TIMER_ID_DRAG);
                stop_move_overlay();
                break;

            case TIMER_ID_WAIT:
                KillTimer(hWnd, TIMER_ID_WAIT);
                ticks = GetTickCount64();
                SetTimer(hWnd, TIMER_ID_FADE, 16, nullptr);
                break;

            case TIMER_ID_FADE: {
                auto &s = mic_muted ? settings.mute_overlay : settings.unmute_overlay;
                uint64 now = GetTickCount64();
                float elapsed = static_cast<float>(now - ticks);
                float duration = static_cast<float>(settings_t::fadeout_over_ms[s.fadeout_speed_ms]);
                int target_alpha = settings_t::fadeout_to_alpha[s.fadeout_to_percent];
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
                show_context_menu(main_hwnd, { LOWORD(wParam), HIWORD(wParam) });
                break;
            }
            break;

        case WM_APP_SHOW_OVERLAY: {
            LOG_DEBUG("WM_APP_SHOW_OVERLAY");
            audio->get_mic_info(&mic_attached, &mic_muted);
            notify_icon.update(mic_attached, mic_muted);
            KillTimer(hWnd, TIMER_ID_WAIT);
            KillTimer(hWnd, TIMER_ID_FADE);
            if(!mic_attached) {
                ShowWindow(hWnd, SW_HIDE);
            } else {

                ShowWindow(hWnd, SW_SHOW);
                update_layered_window(255);
                do_fadeout();
            }
            break;
        }

        case WM_APP_ENDPOINT_CHANGE:
            audio->change_endpoint();
            PostMessage(main_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;

        case WM_APP_HOTKEY_PRESSED:
            audio->toggle_mute();
            break;

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

        if(main_hwnd == nullptr) {
            return WIN32_LAST_ERROR(GetLastError());
        }

        CreateWindowEx(drag_window_ex_flags, drag_window_class_name, window_title, drag_window_flags, 0, 0, 0, 0,
                       nullptr, nullptr, hInst, nullptr);

        // main_hwnd is set in WM_CREATE

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

        if(FAILED(settings.load())) {
            // 1st run, probably... show an about thing?
        }

        hInst = hInstance;

        HR(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
        DEFER(CoUninitialize());

        SetProcessDPIAware();

        WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

        img_size = settings.overlay_position.right - settings.overlay_position.left;

        muted_image.create_from_svg(microphone_mute_svg, img_size, img_size);
        normal_image.create_from_svg(microphone_normal_svg, img_size, img_size);

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
