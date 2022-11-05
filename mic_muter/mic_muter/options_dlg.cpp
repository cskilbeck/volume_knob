#include "framework.h"

namespace
{
    LOG_CONTEXT("options_dlg");

    using namespace chs;

    using mic_muter::settings;
    using mic_muter::settings_t;

    struct dlg_info_t
    {
        int page;
        HWND overlay_dlg;
    };

    settings_t old_settings;

    chs::mic_muter::image overlay_img[chs::mic_muter::num_overlay_ids];

    //////////////////////////////////////////////////////////////////////

    dlg_info_t *dlg_info(HWND w)
    {
        return reinterpret_cast<dlg_info_t *>(GetWindowLongPtr(w, GWLP_USERDATA));
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
            dlg_info_t *info = dlg_info(parent);
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

            for(auto const *text : settings_t::fadeout_after_names) {
                ComboBox_AddString(GetDlgItem(dlg, IDC_COMBO_FADEOUT_AFTER), text);
            }
            for(auto const *text : settings_t::fadeout_to_names) {
                ComboBox_AddString(GetDlgItem(dlg, IDC_COMBO_FADEOUT_TO), text);
            }
            for(auto const *text : settings_t::fadeout_speed_names) {
                ComboBox_AddString(GetDlgItem(dlg, IDC_COMBO_FADEOUT_SPEED), text);
            }

            // setup overlay image icon

            RECT img_rect;
            GetClientRect(GetDlgItem(dlg, IDC_STATIC_OPTIONS_OVERLAY_IMAGE), &img_rect);
            int img_w = img_rect.right - img_rect.left;
            int img_h = img_rect.bottom - img_rect.top;

            for(int i = 0; i < mic_muter::num_overlay_ids; ++i) {
                auto id = static_cast<mic_muter::overlay_id>(i);
                overlay_img[i].create_from_svg(get_overlay_svg(id), img_w, img_h);
            }

            // set up the control values

            setup_option_controls(dlg, &settings.overlay[info->page]);

            break;
        }

        case WM_DESTROY:
            for(auto &img : overlay_img) {
                img.destroy();
            }
            break;

        case WM_DRAWITEM: {

            LPDRAWITEMSTRUCT d = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

            switch(d->CtlID) {

            case IDC_STATIC_OPTIONS_OVERLAY_IMAGE: {

                HDC dc = GetDC(dlg);
                HBRUSH fill_brush = CreateSolidBrush(GetBkColor(dc));
                ReleaseDC(dlg, dc);
                HGDIOBJ old_brush = SelectObject(d->hDC, fill_brush);
                FillRect(d->hDC, &d->rcItem, fill_brush);
                SelectObject(d->hDC, old_brush);
                DeleteObject(fill_brush);

                BLENDFUNCTION bf{};
                bf.BlendOp = AC_SRC_OVER;
                bf.SourceConstantAlpha = 255;
                bf.AlphaFormat = AC_SRC_ALPHA;

                dlg_info_t *info = dlg_info(GetParent(dlg));
                int w = overlay_img[info->page].width;
                int h = overlay_img[info->page].height;
                HDC src_dc = overlay_img[info->page].dc;
                AlphaBlend(d->hDC, 0, 0, w, h, src_dc, 0, 0, w, h, bf);
                break;
            }
            }
        }

        case WM_COMMAND: {
            HWND parent = GetParent(dlg);
            dlg_info_t *info = dlg_info(parent);
            settings_t::overlay_setting *s = &settings.overlay[info->page];
            int action = HIWORD(wParam);
            switch(LOWORD(wParam)) {

            case IDC_CHECK_OVERLAY_ENABLE:
                s->enabled = Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_OVERLAY_ENABLE));
                break;

            case IDC_COMBO_FADEOUT_AFTER:
                if(action == CBN_SELCHANGE) {
                    s->fadeout_time = static_cast<byte>(ComboBox_GetCurSel(GetDlgItem(dlg, IDC_COMBO_FADEOUT_AFTER)));
                }
                break;

            case IDC_COMBO_FADEOUT_SPEED:
                if(action == CBN_SELCHANGE) {
                    s->fadeout_speed = static_cast<byte>(ComboBox_GetCurSel(GetDlgItem(dlg, IDC_COMBO_FADEOUT_SPEED)));
                }
                break;

            case IDC_COMBO_FADEOUT_TO:
                if(action == CBN_SELCHANGE) {
                    s->fadeout_to = static_cast<byte>(ComboBox_GetCurSel(GetDlgItem(dlg, IDC_COMBO_FADEOUT_TO)));
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

    void setup_controls(HWND dlg)
    {
        Button_SetCheck(GetDlgItem(dlg, IDC_CHECK_RUN_AT_STARTUP), settings.run_at_startup);
        int id = chs::mic_muter::get_hotkey_index(hotkey_keycode);
        ComboBox_SetCurSel(GetDlgItem(dlg, IDC_COMBO_HOTKEY), id);
        Button_SetCheck(GetDlgItem(dlg, IDC_CHECK_ALT), (hotkey_modifiers & keymod_alt) != 0);
        Button_SetCheck(GetDlgItem(dlg, IDC_CHECK_CTRL), (hotkey_modifiers & keymod_ctrl) != 0);
        Button_SetCheck(GetDlgItem(dlg, IDC_CHECK_SHIFT), (hotkey_modifiers & keymod_shift) != 0);
        Button_SetCheck(GetDlgItem(dlg, IDC_CHECK_WINKEY), (hotkey_modifiers & keymod_winkey) != 0);
    }

    //////////////////////////////////////////////////////////////////////

    void update_modifiers(BOOL state, int mask)
    {
        if(state) {
            hotkey_modifiers |= mask;
        } else {
            hotkey_modifiers &= ~mask;
        }
    }

    //////////////////////////////////////////////////////////////////////

    void enable_hotkey_controls(HWND dlg, bool enable)
    {
        int show = enable ? SW_HIDE : SW_SHOW;
        int hide = enable ? SW_SHOW : SW_HIDE;
        ShowWindow(GetDlgItem(dlg, IDC_CHECK_ALT), hide);
        ShowWindow(GetDlgItem(dlg, IDC_CHECK_CTRL), hide);
        ShowWindow(GetDlgItem(dlg, IDC_CHECK_SHIFT), hide);
        ShowWindow(GetDlgItem(dlg, IDC_CHECK_WINKEY), hide);
        ShowWindow(GetDlgItem(dlg, IDC_COMBO_HOTKEY), hide);
        ShowWindow(GetDlgItem(dlg, IDC_STATIC_HOTKEY_MESSAGE), show);
        SetWindowText(GetDlgItem(dlg, IDC_BUTTON_CHOOSE_HOTKEY), enable ? "Scan" : "Cancel");
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK options_dlg_proc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {

            // keyboard hook sends messages to this dialog, needs to know where it is
            options_dlg = dlg;

            // if dialog is cancelled, go back to these settings
            old_settings = settings;

            // init the tab control tabs' text
            HWND tab_ctrl = GetDlgItem(dlg, IDC_OPTIONS_TAB_CTRL);
            TCITEM tie{};
            tie.mask = TCIF_TEXT;
            for(int i = 0; i < mic_muter::num_overlay_ids; ++i) {
                tie.pszText = const_cast<char *>(mic_muter::get_overlay_name(i));
                TabCtrl_InsertItem(tab_ctrl, i, &tie);
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
            dlg_info_t *info = new dlg_info_t;
            info->page = static_cast<int>(lParam);
            SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)info);

            CreateDialogIndirect(GetModuleHandle(nullptr), dlg_template, dlg, overlay_dlg_proc);

            HWND combo = GetDlgItem(dlg, IDC_COMBO_HOTKEY);
            for(int i = 0; i < chs::mic_muter::num_hotkeys; ++i) {
                chs::mic_muter::hotkey_t &hotkey = chs::mic_muter::hotkeys[i];
                ComboBox_AddString(combo, hotkey.name);
            }

            setup_controls(dlg);

            // show the requested overlay page
            TabCtrl_SetCurSel(tab_ctrl, lParam);

            break;
        }

        case WM_DESTROY: {
            delete dlg_info(dlg);
            PostMessage(overlay_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;
        }

        case WM_ACTIVATE:
            if(wParam == WA_INACTIVE) {
                hotkey_scanning = false;
                enable_hotkey_controls(dlg, !hotkey_scanning);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDCANCEL:
                settings = old_settings;
                hotkey_keycode = settings.hotkey;
                hotkey_modifiers = settings.modifiers;
                EndDialog(dlg, IDCANCEL);
                break;
            case IDOK:
                settings.modifiers = hotkey_modifiers;
                settings.hotkey = hotkey_keycode;
                settings.save();
                EndDialog(dlg, IDCANCEL);
                break;
            case IDC_CHECK_RUN_AT_STARTUP:
                settings.run_at_startup = Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_RUN_AT_STARTUP)) != 0;
                break;
            case IDC_BUTTON_CHOOSE_HOTKEY:
                hotkey_scanning = !hotkey_scanning;
                enable_hotkey_controls(dlg, !hotkey_scanning);
                break;
            case IDC_COMBO_HOTKEY: {
                HWND combo = GetDlgItem(dlg, IDC_COMBO_HOTKEY);
                int id = std::clamp(ComboBox_GetCurSel(combo), 0, static_cast<int>(chs::mic_muter::num_hotkeys));
                hotkey_keycode = chs::mic_muter::hotkeys[id].key_code;
                LOG_DEBUG("NEW CODE: ID: {}, CODE: {}", id, hotkey_keycode);
                break;
            }
            case IDC_CHECK_ALT:
                update_modifiers(Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_ALT)), keymod_alt);
                break;
            case IDC_CHECK_CTRL:
                update_modifiers(Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_CTRL)), keymod_ctrl);
                break;
            case IDC_CHECK_SHIFT:
                update_modifiers(Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_SHIFT)), keymod_shift);
                break;
            case IDC_CHECK_WINKEY:
                update_modifiers(Button_GetCheck(GetDlgItem(dlg, IDC_CHECK_WINKEY)), keymod_winkey);
                break;
            }
            break;

        case WM_APP_HOTKEY_PRESSED:
            LOG_DEBUG("New hotkey: {:02x}, {:02x}", wParam, lParam);
            enable_hotkey_controls(dlg, true);
            ShowWindow(GetDlgItem(dlg, IDC_STATIC_HOTKEY_MESSAGE), SW_HIDE);
            setup_controls(dlg);
            break;

        case WM_NOTIFY: {
            LPNMHDR n = reinterpret_cast<LPNMHDR>(lParam);
            switch(n->idFrom) {
            case IDC_OPTIONS_TAB_CTRL:
                if(n->code == TCN_SELCHANGE) {
                    HWND tab_ctrl = GetDlgItem(dlg, IDC_OPTIONS_TAB_CTRL);
                    int page = TabCtrl_GetCurSel(tab_ctrl);
                    LOG_DEBUG("page: {}", page);
                    dlg_info_t *info = dlg_info(dlg);
                    info->page = std::clamp(page, 0, mic_muter::max_overlay_id);
                    setup_option_controls(info->overlay_dlg, &settings.overlay[page]);
                }
                break;
            }
            break;
        }
        }
        return FALSE;
    }
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    HRESULT show_options_dialog(bool muted, bool attached)
    {
        HWND w = FindWindow("#32770", "MicMuter options");
        if(w != nullptr) {
            BringWindowToTop(w);
            SetForegroundWindow(w);
            SetActiveWindow(w);
            SwitchToThisWindow(w, false);
            return S_OK;
        }
        LPSTR dlg_template = MAKEINTRESOURCE(IDD_DIALOG_OPTIONS);
        int page = get_overlay_id(muted, attached);
        DialogBoxParam(GetModuleHandle(nullptr), dlg_template, overlay_hwnd, options_dlg_proc, page);
        return S_OK;
    }
}
