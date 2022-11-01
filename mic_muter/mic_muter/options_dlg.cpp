#include "framework.h"

namespace
{
    LOG_CONTEXT("options_dlg");

    struct dlg_info
    {
        int page;
        HWND overlay_dlg;
    };
    chs::mic_muter::settings_t cancel_settings;

    HWND options_dlg{ nullptr };

    int constexpr is_muted = 1;
    int constexpr is_attached = 2;
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    dlg_info *get_info(HWND w)
    {
        return reinterpret_cast<dlg_info *>(GetWindowLongPtr(w, GWLP_USERDATA));
    }

    //////////////////////////////////////////////////////////////////////

    HWND Ctrl(HWND w, int id)
    {
        return GetDlgItem(w, id);
    }

    //////////////////////////////////////////////////////////////////////

    void setup_option_controls(HWND w, settings_t::overlay_setting *s)
    {
        Button_SetCheck(Ctrl(w, IDC_CHECK_OVERLAY_ENABLE), s->enabled);
        ComboBox_SetCurSel(Ctrl(w, IDC_COMBO_FADEOUT_AFTER), s->fadeout_time_ms);
        ComboBox_SetCurSel(Ctrl(w, IDC_COMBO_FADEOUT_TO), s->fadeout_to_percent);
        ComboBox_SetCurSel(Ctrl(w, IDC_COMBO_FADEOUT_SPEED), s->fadeout_speed_ms);
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK overlay_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {
            HWND parent = GetParent(hwndDlg);
            dlg_info *info = get_info(parent);
            info->overlay_dlg = hwndDlg;
            HWND tab_ctrl = GetDlgItem(parent, IDC_OPTIONS_TAB_CTRL);
            RECT rc;
            GetWindowRect(tab_ctrl, &rc);
            MapWindowPoints(nullptr, parent, (LPPOINT)&rc, 2);
            TabCtrl_AdjustRect(tab_ctrl, false, &rc);
            SetWindowPos(hwndDlg, nullptr, rc.left, rc.top, 0, 0, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
            for(auto const *text : settings.fadeout_after_ms_names) {
                ComboBox_AddString(Ctrl(hwndDlg, IDC_COMBO_FADEOUT_AFTER), text);
            }
            for(auto const *text : settings.fadeout_to_names) {
                ComboBox_AddString(Ctrl(hwndDlg, IDC_COMBO_FADEOUT_TO), text);
            }
            for(auto const *text : settings.fadeout_speed_names) {
                ComboBox_AddString(Ctrl(hwndDlg, IDC_COMBO_FADEOUT_SPEED), text);
            }
            setup_option_controls(hwndDlg, settings_t::overlay_settings[info->page]);
            break;
        }

        case WM_COMMAND: {
            HWND parent = GetParent(hwndDlg);
            dlg_info *info = get_info(parent);
            settings_t::overlay_setting *s = settings_t::overlay_settings[info->page];
            bool changed{ false };
            switch(LOWORD(wParam)) {
            case IDC_CHECK_OVERLAY_ENABLE:
                s->enabled = Button_GetCheck(Ctrl(hwndDlg, IDC_CHECK_OVERLAY_ENABLE));
                changed = true;
                break;
            case IDC_COMBO_FADEOUT_AFTER:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    s->fadeout_time_ms = ComboBox_GetCurSel(Ctrl(hwndDlg, IDC_COMBO_FADEOUT_AFTER));
                    changed = true;
                }
                break;
            case IDC_COMBO_FADEOUT_SPEED:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    s->fadeout_speed_ms = ComboBox_GetCurSel(Ctrl(hwndDlg, IDC_COMBO_FADEOUT_SPEED));
                    changed = true;
                }
                break;
            case IDC_COMBO_FADEOUT_TO:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    s->fadeout_to_percent = ComboBox_GetCurSel(Ctrl(hwndDlg, IDC_COMBO_FADEOUT_TO));
                    changed = true;
                }
                break;
            }
            if(changed) {
                LOG_INFO("PAGE: {}, enabled: {}, fadeout_time_ms: {}, fadeout_to_percent: {}, fadeout_speed_ms: {}",
                         info->page, s->enabled, s->fadeout_time_ms, s->fadeout_to_percent, s->fadeout_speed_ms);
            }
            break;
        }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK options_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {
            options_dlg = hwndDlg;

            INITCOMMONCONTROLSEX iccex{};
            iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            iccex.dwICC = ICC_TAB_CLASSES;
            InitCommonControlsEx(&iccex);

            // if dialog is cancelled, go back to these settings
            cancel_settings = settings;

            // init the tab control tabs' text
            HWND tab_ctrl = GetDlgItem(hwndDlg, IDC_OPTIONS_TAB_CTRL);
            TCITEM tie{};
            tie.mask = TCIF_TEXT;
            int id = 0;
            for(auto const s : settings_t::overlay_settings) {
                tie.pszText = const_cast<char *>(s->name);
                TabCtrl_InsertItem(tab_ctrl, id, &tie);
                id += 1;
            }

            // create the overlay_settings child dialog
            HRSRC hrsrc = FindResource(NULL, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS_OVERLAY), RT_DIALOG);
            if(hrsrc == nullptr) {
                return false;
            }
            HGLOBAL hglb = LoadResource(GetModuleHandle(nullptr), hrsrc);
            if(hglb == nullptr) {
                return false;
            }
            DEFER(FreeResource(hglb));
            DLGTEMPLATE *dlg_template = reinterpret_cast<DLGTEMPLATE *>(LockResource(hglb));
            if(dlg_template == nullptr) {
                return false;
            }
            DEFER(UnlockResource(dlg_template));

            // attach some tracking info to the options dialog
            dlg_info *info = new dlg_info;
            info->page = static_cast<int>(lParam);
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)info);

            CreateDialogIndirect(GetModuleHandle(nullptr), dlg_template, hwndDlg, overlay_dlg_proc);

            // show the requested overlay page
            TabCtrl_SetCurSel(tab_ctrl, lParam);
            Button_SetCheck(Ctrl(hwndDlg, IDC_CHECK_RUN_AT_STARTUP), settings.run_at_startup);
            break;
        }

        case WM_DESTROY: {
            dlg_info *info = get_info(hwndDlg);
            delete info;
            options_dlg = nullptr;
            PostMessage(overlay_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDCANCEL:
                settings = cancel_settings;
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                settings.save();
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDC_CHECK_RUN_AT_STARTUP:
                settings.run_at_startup = Button_GetCheck(Ctrl(hwndDlg, IDC_CHECK_RUN_AT_STARTUP)) != 0;
                break;
            }
            break;

        case WM_NOTIFY: {
            LPNMHDR n = (LPNMHDR)lParam;
            switch(n->idFrom) {
            case IDC_OPTIONS_TAB_CTRL:
                if(n->code == TCN_SELCHANGE) {
                    HWND tab_ctrl = GetDlgItem(hwndDlg, IDC_OPTIONS_TAB_CTRL);
                    int page = TabCtrl_GetCurSel(tab_ctrl);
                    LOG_DEBUG("page: {}", page);
                    dlg_info *info = get_info(hwndDlg);
                    info->page = std::clamp(page, 0, static_cast<int>(settings_t::num_overlay_pages));
                    setup_option_controls(info->overlay_dlg, settings_t::overlay_settings[page]);
                }
                break;
            }
            break;
        }
        }
        return FALSE;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT show_options_dialog(bool muted, bool attached)
    {
        if(options_dlg != nullptr) {
            SetForegroundWindow(options_dlg);
            SetActiveWindow(options_dlg);
            SwitchToThisWindow(options_dlg, false);
        } else {
            DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_DIALOG_OPTIONS), overlay_hwnd,
                           options_dlg_proc, settings_t::get_overlay_index(muted, attached));
        }
        return S_OK;
    }
}
