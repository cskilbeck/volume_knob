//////////////////////////////////////////////////////////////////////
// keyboard hook
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

namespace chs
{
    LOG_CONTEXT("mic_muter");

    //////////////////////////////////////////////////////////////////////

    HWND main_hwnd = nullptr;
    HINSTANCE hInst;

    ComPtr<audio_controller> audio;

    notification_icon notify_icon;

    bool double_buffered = false;
    float current_dpi = 1.0f;

    VOLUME_INFO current_volume{};

    HBITMAP muted_bmp;
    HBITMAP non_muted_bmp;
    HDC bmp_dc;

    int img_size;

    int window_alpha = 255;

    UINT_PTR timer_id = 101;

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

    void show_context_menu(HWND hwnd, POINT const &pt)
    {
        HMENU hMenu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MENU_POPUP));
        if(hMenu) {
            HMENU hSubMenu = GetSubMenu(hMenu, 0);
            if(hSubMenu) {
                SetForegroundWindow(hwnd);
                UINT uFlags = TPM_RIGHTBUTTON;
                if(GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
                    uFlags |= TPM_RIGHTALIGN;
                } else {
                    uFlags |= TPM_LEFTALIGN;
                }
                TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
            }
            DestroyMenu(hMenu);
        }
    }

    //////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {

        case WM_CREATE: {

            main_hwnd = hWnd;

            // Make BLACK the transparency color and use 25% alpha
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 64, LWA_ALPHA | LWA_COLORKEY);

            init_dpi_scale(hWnd);

            // Position at the center of the primary monitor
            POINT const ptZeroZero = {};
            HMONITOR hMonitor = MonitorFromPoint(ptZeroZero, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hMonitor, &mi);

            img_size = (mi.rcMonitor.right - mi.rcMonitor.left) * 5 / 100;

            SIZE const size = { img_size, img_size };

            POINT const pt = { mi.rcMonitor.left + img_size, mi.rcMonitor.bottom - size.cy * 2 };

            SetWindowPos(hWnd, HWND_TOPMOST, pt.x, pt.y, size.cx, size.cy, 0);

            bmp_dc = CreateCompatibleDC(nullptr);

            util::load_bitmap(ID_IMG_MUTED, &muted_bmp, img_size, img_size);
            util::load_bitmap(ID_IMG_NOT_MUTED, &non_muted_bmp, img_size, img_size);

            break;
        }

        case WM_DESTROY: {
            notify_icon.destroy();
            UnhookWindowsHookEx(mic_mute_hook);
            DeleteDC(bmp_dc);
            PostQuitMessage(0);
            return 0;
        }

        case WM_ERASEBKGND: {
            return 1;
        }

        case WM_COMMAND: {
            int const wmId = LOWORD(wParam);
            switch(wmId) {
            case ID_POPUP_QUIT:
                DestroyWindow(main_hwnd);
                break;
            }
            break;
        }

        case WM_NOTIFICATION_ICON: {

            switch(LOWORD(lParam)) {
            case WM_CONTEXTMENU:
                show_context_menu(main_hwnd, { LOWORD(wParam), HIWORD(wParam) });
                break;
            }
            break;
        }

        case WM_VOLUMECHANGE: {
            audio->get_level_info(&current_volume);
            notify_icon.update(current_volume.bMuted);
            window_alpha = 255;
            SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), window_alpha, LWA_ALPHA | LWA_COLORKEY);
            ShowWindow(hWnd, SW_SHOW);
            InvalidateRect(hWnd, nullptr, TRUE);
            timer_id = SetTimer(hWnd, 101, 33, nullptr);
            return 0;
        }

        case WM_ENDPOINTCHANGE: {
            audio->change_endpoint();
            return 0;
        }

        case WM_TIMER: {
            window_alpha -= 1;
            if(window_alpha != 0) {
                SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), window_alpha, LWA_ALPHA | LWA_COLORKEY);
            } else {
                ShowWindow(hWnd, SW_HIDE);
                KillTimer(hWnd, timer_id);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HPAINTBUFFER hBufferedPaint = nullptr;
            RECT rc;


            GetClientRect(hWnd, &rc);
            HDC hdc = BeginPaint(hWnd, &ps);

            if(double_buffered) {
                HDC hdcMem;
                hBufferedPaint = BeginBufferedPaint(hdc, &rc, BPBF_COMPOSITED, nullptr, &hdcMem);
                if(hBufferedPaint) {
                    hdc = hdcMem;
                }
            }

            FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

            HBITMAP old_bmp = SelectBitmap(bmp_dc, current_volume.bMuted ? muted_bmp : non_muted_bmp);
            BitBlt(hdc, 0, 0, img_size, img_size, bmp_dc, 0, 0, SRCPAINT);
            SelectBitmap(bmp_dc, old_bmp);

            if(hBufferedPaint) {
                BufferedPaintMakeOpaque(hBufferedPaint, nullptr);
                EndBufferedPaint(hBufferedPaint, true);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT init_window()
    {
        HR(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));

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

        hInst = hInstance;

        SetProcessDPIAware();

        HR(init_window());

        mic_mute_hook = SetWindowsHookEx(WH_KEYBOARD_LL, mic_mute_hook_function, hInstance, 0);

        audio.Attach(new(std::nothrow) chs::audio_controller());

        // OK if this fails (e.g. microphone unplugged - if it gets plugged in it will pick it up)
        audio->init();

        audio->get_level_info(&current_volume);

        notify_icon.load(current_volume.bMuted);

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(double_buffered) {
            BufferedPaintUnInit();
        }

        audio->Dispose();
        audio.Reset();

        CoUninitialize();

        return S_OK;
    }
}

//////////////////////////////////////////////////////////////////////

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    LOG_CONTEXT("wWinMain");

    HRESULT hr = chs::win_main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    if(FAILED(hr)) {
        LOG_ERROR("win_main() failed: {}", chs::util::windows_error_text(hr));
    }
    return 0;
}