//////////////////////////////////////////////////////////////////////
// startup registry entry
// options
// installer
//////////////////////////////////////////////////////////////////////

#include "framework.h"
#include "mic_muter.h"

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

#pragma comment(linker,                                                                                         \
                "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0'" \
                " processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    LOG_CONTEXT("mic_muter");

    //////////////////////////////////////////////////////////////////////

    HINSTANCE hInst;

    ComPtr<audio_controller> audio;

    notification_icon notify_icon;

    bool is_muted;
    bool is_attached;

    bool double_buffered = false;
    float current_dpi = 1.0f;

    HBITMAP muted_bmp;
    HBITMAP non_muted_bmp;
    HDC bmp_dc;

    int img_size;

    int window_alpha = 255;
    int min_window_alpha = 0;

    UINT_PTR const TIMER_ID_WAIT = 101;
    UINT_PTR const TIMER_ID_FADE = 102;

    //////////////////////////////////////////////////////////////////////

    void init_dpi_scale(HWND hWnd)
    {
        HDC hdc = GetDC(hWnd);
        current_dpi = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
        ReleaseDC(hWnd, hdc);
    }

    //////////////////////////////////////////////////////////////////////

    int dpi_scale(int iValue)
    {
        return static_cast<int>(static_cast<float>(iValue) * current_dpi);
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT show_context_menu(HWND hwnd, POINT const &pt)
    {
        bool present{ true };
        bool muted{ false };
        HR(audio->get_mic_info(&present, &muted));
        HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MENU_POPUP));
        if(hMenu == nullptr) {
            return WIN32_ERROR("LoadMenu");
        }
        DEFER(DestroyMenu(hMenu));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if(hSubMenu == nullptr) {
            return WIN32_ERROR("GetSubMenu");
        }

        MENUITEMINFO mi;
        mi.cbSize = sizeof(MENUITEMINFO);
        mi.fMask = MIIM_STATE | MIIM_STRING;
        mi.fState = present ? MFS_ENABLED : MFS_DISABLED;
        mi.dwTypeData = const_cast<LPSTR>(muted ? "Unmute" : "Mute");
        if(!SetMenuItemInfo(hSubMenu, ID_POPUP_MUTE, false, &mi)) {
            return WIN32_ERROR("SetMenuItemInfo");
        }
        SetForegroundWindow(hwnd);
        UINT uFlags = TPM_RIGHTBUTTON;
        if(GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
            uFlags |= TPM_RIGHTALIGN;
        } else {
            uFlags |= TPM_LEFTALIGN;
        }
        if(!TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL)) {
            return WIN32_ERROR("TrackPopupMenuEx");
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    void draw_image(HWND hWnd, HDC hdc)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        HPAINTBUFFER hBufferedPaint = nullptr;
        if(double_buffered) {
            HDC hdcMem;
            hBufferedPaint = BeginBufferedPaint(hdc, &rc, BPBF_COMPOSITED, nullptr, &hdcMem);
            if(hBufferedPaint) {
                hdc = hdcMem;
            }
        }

        FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

        HBITMAP old_bmp = SelectBitmap(bmp_dc, is_muted ? muted_bmp : non_muted_bmp);
        BitBlt(hdc, 0, 0, img_size, img_size, bmp_dc, 0, 0, SRCPAINT);
        SelectBitmap(bmp_dc, old_bmp);

        if(hBufferedPaint) {
            BufferedPaintMakeOpaque(hBufferedPaint, nullptr);
            EndBufferedPaint(hBufferedPaint, true);
        }
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK options_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }
            break;
        }
        return FALSE;
    }

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {

        case WM_CREATE: {

            // Make BLACK the transparency color and use 25% alpha
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 64, LWA_ALPHA | LWA_COLORKEY);

            init_dpi_scale(hWnd);

            HMONITOR hMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hMonitor, &mi);

            img_size = (mi.rcMonitor.right - mi.rcMonitor.left) * 5 / 100;

            POINT const pt = { mi.rcMonitor.left + img_size, mi.rcMonitor.bottom - img_size * 2 };

            SetWindowPos(hWnd, HWND_TOPMOST, pt.x, pt.y, img_size, img_size, 0);

            bmp_dc = CreateCompatibleDC(nullptr);

            util::load_bitmap(ID_IMG_MUTED, &muted_bmp, img_size, img_size);
            util::load_bitmap(ID_IMG_NOT_MUTED, &non_muted_bmp, img_size, img_size);

            main_hwnd = hWnd;

            audio.Attach(new(std::nothrow) audio_controller());

            mic_mute_hook = SetWindowsHookEx(WH_KEYBOARD_LL, mic_mute_hook_function, hInst, 0);

            audio->init();

            audio->get_mic_info(&is_attached, &is_muted);

            notify_icon.load();
            notify_icon.update(is_attached, is_muted);

            // check here if they want the overlay to be shown based on mute
            // status
            PostMessage(hWnd, WM_VOLUMECHANGE, 0, 0);

            break;
        }

        case WM_DESTROY:
            notify_icon.destroy();
            UnhookWindowsHookEx(mic_mute_hook);
            DeleteDC(bmp_dc);
            if(double_buffered) {
                BufferedPaintUnInit();
            }
            audio->Dispose();
            audio.Reset();
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND: {
            int const wmId = LOWORD(wParam);
            switch(wmId) {
            case ID_POPUP_QUIT:
                DestroyWindow(main_hwnd);
                break;
            case ID_POPUP_OPTIONS:
                DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), main_hwnd, options_dlg_proc);
                break;
            case ID_POPUP_MUTE:
                audio->toggle_mute();
                break;
            }
            break;
        }

        case WM_NOTIFICATION_ICON:

            switch(LOWORD(lParam)) {
            case WM_CONTEXTMENU:
                show_context_menu(main_hwnd, { LOWORD(wParam), HIWORD(wParam) });
                break;
            }
            break;

        case WM_VOLUMECHANGE: {
            audio->get_mic_info(&is_attached, &is_muted);
            notify_icon.update(is_attached, is_muted);
            KillTimer(hWnd, TIMER_ID_WAIT);
            KillTimer(hWnd, TIMER_ID_FADE);
            if(!is_attached) {
                ShowWindow(hWnd, SW_HIDE);
            } else {
                window_alpha = 255;
                min_window_alpha = is_muted ? 64 : 0;
                SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), window_alpha, LWA_ALPHA | LWA_COLORKEY);
                HDC dc = GetDC(hWnd);
                draw_image(hWnd, dc);
                ReleaseDC(hWnd, dc);
                ShowWindow(hWnd, SW_SHOW);
                SetTimer(hWnd, TIMER_ID_WAIT, 1000 * 3, nullptr);
            }
            return 0;
        }

        case WM_MIC_STATE_CHANGED:
            break;

        case WM_ENDPOINTCHANGE:
            audio->change_endpoint();
            PostMessage(main_hwnd, WM_VOLUMECHANGE, 0, 0);
            return 0;

        case WM_TIMER: {
            switch(wParam) {
            case TIMER_ID_WAIT:
                KillTimer(hWnd, TIMER_ID_WAIT);
                SetTimer(hWnd, TIMER_ID_FADE, 16, nullptr);
                break;
            case TIMER_ID_FADE:
                window_alpha = std::max(min_window_alpha, window_alpha - 16);
                if(window_alpha != 0) {
                    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), std::min(255, window_alpha),
                                               LWA_ALPHA | LWA_COLORKEY);
                    ShowWindow(hWnd, SW_SHOW);
                } else {
                    KillTimer(hWnd, TIMER_ID_FADE);
                    ShowWindow(hWnd, SW_HIDE);
                }
                break;
            }
            return 0;
        }

        case WM_HOTKEY_PRESSED:
            LOG_INFO("HOTKEY!");
            audio->toggle_mute();
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            draw_image(hWnd, hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT init_window()
    {
        double_buffered = SUCCEEDED(BufferedPaintInit());

        constexpr char class_name[] = "mic_muter";

        WNDCLASSEX wcex = { sizeof(wcex) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInst;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszClassName = class_name;

        if(RegisterClassEx(&wcex) == 0) {
            return WIN32_ERROR(GetLastError());
        }

        DWORD const style = WS_POPUP;
        DWORD const ex_style = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;
        CreateWindowEx(ex_style, class_name, nullptr, style, 0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);

        if(main_hwnd == nullptr) {
            return WIN32_ERROR(GetLastError());
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT win_main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
    {
        UNREFERENCED_PARAMETER(hPrevInstance);
        UNREFERENCED_PARAMETER(lpCmdLine);

        BOOL suppress = true;
        SetUserObjectInformationW(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &suppress,
                                  sizeof(suppress));

        hInst = hInstance;

        HR(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));

        SetProcessDPIAware();

        HR(init_window());

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        CoUninitialize();

        return S_OK;
    }
}

//////////////////////////////////////////////////////////////////////

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    LOG_CONTEXT("wWinMain");

    HRESULT hr = chs::mic_muter::win_main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    if(FAILED(hr)) {
        LOG_ERROR("win_main() failed: {}", chs::util::windows_error_text(hr));
    }
    return 0;
}
