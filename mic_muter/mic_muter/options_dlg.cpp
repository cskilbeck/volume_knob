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

    chs::mic_muter::image overlay_img[chs::mic_muter::num_overlay_ids];
    int img_w;
    int img_h;
    HBRUSH overlay_back_brush;
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    dlg_info *get_info(HWND w)
    {
        return reinterpret_cast<dlg_info *>(GetWindowLongPtr(w, GWLP_USERDATA));
    }

    //////////////////////////////////////////////////////////////////////

    void enable_option_controls(HWND w, settings_t::overlay_setting *s)
    {
        bool enable = s->enabled;
        EnableWindow(GetDlgItem(w, IDC_COMBO_FADEOUT_AFTER), enable);

        enable &= s->fadeout_time != settings_t::fadeout_after::fadeout_never;
        EnableWindow(GetDlgItem(w, IDC_COMBO_FADEOUT_TO), enable);
        EnableWindow(GetDlgItem(w, IDC_COMBO_FADEOUT_SPEED), enable);
    }

    //////////////////////////////////////////////////////////////////////

    void setup_option_controls(HWND w, settings_t::overlay_setting *s)
    {
        Button_SetCheck(GetDlgItem(w, IDC_CHECK_OVERLAY_ENABLE), s->enabled);
        ComboBox_SetCurSel(GetDlgItem(w, IDC_COMBO_FADEOUT_AFTER), s->fadeout_time);
        ComboBox_SetCurSel(GetDlgItem(w, IDC_COMBO_FADEOUT_TO), s->fadeout_to);
        ComboBox_SetCurSel(GetDlgItem(w, IDC_COMBO_FADEOUT_SPEED), s->fadeout_speed);
        enable_option_controls(w, s);
        InvalidateRect(GetDlgItem(w, IDC_STATIC_OPTIONS_OVERLAY_IMAGE), nullptr, false);
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK overlay_dlg_proc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {

            // fill in main dialog info block

            HWND parent = GetParent(dlg);
            dlg_info *info = get_info(parent);
            info->overlay_dlg = dlg;

            // setup position within the tab control

            HWND tab_ctrl = GetDlgItem(parent, IDC_OPTIONS_TAB_CTRL);
            RECT rc;
            GetWindowRect(tab_ctrl, &rc);
            MapWindowPoints(nullptr, parent, reinterpret_cast<LPPOINT>(&rc), 2);
            TabCtrl_AdjustRect(tab_ctrl, false, &rc);
            SetWindowPos(dlg, nullptr, rc.left, rc.top, 0, 0, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
            EnableThemeDialogTexture(dlg, ETDT_ENABLETAB);

            // add strings to the combo boxes

            for(auto const *text : settings.fadeout_after_names) {
                ComboBox_AddString(GetDlgItem(dlg, IDC_COMBO_FADEOUT_AFTER), text);
            }
            for(auto const *text : settings.fadeout_to_names) {
                ComboBox_AddString(GetDlgItem(dlg, IDC_COMBO_FADEOUT_TO), text);
            }
            for(auto const *text : settings.fadeout_speed_names) {
                ComboBox_AddString(GetDlgItem(dlg, IDC_COMBO_FADEOUT_SPEED), text);
            }

            // setup overlay image icon

            RECT img_rect;
            GetClientRect(GetDlgItem(dlg, IDC_STATIC_OPTIONS_OVERLAY_IMAGE), &img_rect);
            img_w = img_rect.right - img_rect.left;
            img_h = img_rect.bottom - img_rect.top;

            for(int i = 0; i < num_overlay_ids; ++i) {
                auto id = static_cast<overlay_id>(i);
                overlay_img[i].create_from_svg(get_svg(id), img_w, img_h);
            }

            HDC dc = GetDC(dlg);
            overlay_back_brush = CreateSolidBrush(GetBkColor(dc));
            ReleaseDC(dlg, dc);

            // set up the control values

            setup_option_controls(dlg, settings_t::overlay_settings[info->page]);

            break;
        }

        case WM_DESTROY:
            for(auto &img : overlay_img) {
                img.destroy();
            }
            DeleteObject(reinterpret_cast<HGDIOBJ *>(overlay_back_brush));
            break;

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT d = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

            HGDIOBJ old_brush = SelectObject(d->hDC, overlay_back_brush);
            FillRect(d->hDC, &d->rcItem, overlay_back_brush);
            SelectObject(d->hDC, old_brush);

            BLENDFUNCTION bf{};
            bf.BlendOp = AC_SRC_OVER;
            bf.SourceConstantAlpha = 255;
            bf.AlphaFormat = AC_SRC_ALPHA;

            dlg_info *info = get_info(GetParent(dlg));
            int page = std::clamp(info->page, 0, static_cast<int>(num_overlay_ids));

            AlphaBlend(d->hDC, 0, 0, img_w, img_h, overlay_img[page].dc, 0, 0, img_w, img_h, bf);
            break;
        }

        case WM_COMMAND: {
            HWND parent = GetParent(dlg);
            dlg_info *info = get_info(parent);
            settings_t::overlay_setting *s = settings_t::overlay_settings[info->page];
            int action = HIWORD(wParam);
            switch(LOWORD(wParam)) {

            case IDC_CHECK_OVERLAY_ENABLE:
                s->enabled = Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_OVERLAY_ENABLE));
                break;

            case IDC_COMBO_FADEOUT_AFTER:
                if(action == CBN_SELCHANGE) {
                    s->fadeout_time = ComboBox_GetCurSel(GetDlgItem(dlg, IDC_COMBO_FADEOUT_AFTER));
                }
                break;

            case IDC_COMBO_FADEOUT_SPEED:
                if(action == CBN_SELCHANGE) {
                    s->fadeout_speed = ComboBox_GetCurSel(GetDlgItem(dlg, IDC_COMBO_FADEOUT_SPEED));
                }
                break;

            case IDC_COMBO_FADEOUT_TO:
                if(action == CBN_SELCHANGE) {
                    s->fadeout_to = ComboBox_GetCurSel(GetDlgItem(dlg, IDC_COMBO_FADEOUT_TO));
                }
                break;
            }
            enable_option_controls(dlg, s);
            break;
        }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK options_dlg_proc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {
            options_dlg = dlg;

            INITCOMMONCONTROLSEX iccex{};
            iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            iccex.dwICC = ICC_TAB_CLASSES;
            InitCommonControlsEx(&iccex);

            // if dialog is cancelled, go back to these settings
            cancel_settings = settings;

            // init the tab control tabs' text
            HWND tab_ctrl = GetDlgItem(dlg, IDC_OPTIONS_TAB_CTRL);
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
            SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)info);

            CreateDialogIndirect(GetModuleHandle(nullptr), dlg_template, dlg, overlay_dlg_proc);

            // show the requested overlay page
            TabCtrl_SetCurSel(tab_ctrl, lParam);
            Button_SetCheck(GetDlgItem(dlg, IDC_CHECK_RUN_AT_STARTUP), settings.run_at_startup);
            break;
        }

        case WM_DESTROY: {
            dlg_info *info = get_info(dlg);
            delete info;
            options_dlg = nullptr;
            PostMessage(overlay_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDCANCEL:
                settings = cancel_settings;
                EndDialog(dlg, IDCANCEL);
                break;
            case IDOK:
                settings.save();
                EndDialog(dlg, IDCANCEL);
                break;
            case IDC_CHECK_RUN_AT_STARTUP:
                settings.run_at_startup = Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_RUN_AT_STARTUP)) != 0;
                break;
            }
            break;

        case WM_NOTIFY: {
            LPNMHDR n = reinterpret_cast<LPNMHDR>(lParam);
            switch(n->idFrom) {
            case IDC_OPTIONS_TAB_CTRL:
                if(n->code == TCN_SELCHANGE) {
                    HWND tab_ctrl = GetDlgItem(dlg, IDC_OPTIONS_TAB_CTRL);
                    int page = TabCtrl_GetCurSel(tab_ctrl);
                    LOG_DEBUG("page: {}", page);
                    dlg_info *info = get_info(dlg);
                    info->page = std::clamp(page, 0, static_cast<int>(num_overlay_ids));
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
            BringWindowToTop(options_dlg);
            SetForegroundWindow(options_dlg);
            SetActiveWindow(options_dlg);
            SwitchToThisWindow(options_dlg, false);
        } else {
            LPSTR dlg_template = MAKEINTRESOURCE(IDD_DIALOG_OPTIONS);
            int page = get_overlay_id(muted, attached);
            DialogBoxParam(GetModuleHandle(nullptr), dlg_template, overlay_hwnd, options_dlg_proc, page);
        }
        return S_OK;
    }
}
