#include "framework.h"

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    HWND Ctrl(int id)
    {
        return GetDlgItem(options_dlg, id);
    }

    //////////////////////////////////////////////////////////////////////

    void setup_option_controls()
    {
        Button_SetCheck(Ctrl(IDC_CHECK_RUN_AT_STARTUP), settings.run_at_startup);
        Button_SetCheck(Ctrl(IDC_CHECK_OVERLAY_ENABLE_MUTE), settings.mute_overlay.enabled);
        Button_SetCheck(Ctrl(IDC_CHECK_OVERLAY_ENABLE_UNMUTE), settings.unmute_overlay.enabled);

        ComboBox_SetCurSel(Ctrl(IDC_COMBO_FADEOUT_AFTER_MUTE), settings.mute_overlay.fadeout_time_ms);
        ComboBox_SetCurSel(Ctrl(IDC_COMBO_FADEOUT_AFTER_UNMUTE), settings.unmute_overlay.fadeout_time_ms);
        ComboBox_SetCurSel(Ctrl(IDC_COMBO_FADEOUT_TO_MUTE), settings.mute_overlay.fadeout_to_percent);
        ComboBox_SetCurSel(Ctrl(IDC_COMBO_FADEOUT_TO_UNMUTE), settings.unmute_overlay.fadeout_to_percent);
        ComboBox_SetCurSel(Ctrl(IDC_COMBO_FADEOUT_SPEED_MUTE), settings.mute_overlay.fadeout_speed_ms);
        ComboBox_SetCurSel(Ctrl(IDC_COMBO_FADEOUT_SPEED_UNMUTE), settings.unmute_overlay.fadeout_speed_ms);
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK options_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {

        case WM_INITDIALOG:
            options_dlg = hwndDlg;
            for(auto const *text : settings.fadeout_after_ms_names) {
                ComboBox_AddString(Ctrl(IDC_COMBO_FADEOUT_AFTER_MUTE), text);
                ComboBox_AddString(Ctrl(IDC_COMBO_FADEOUT_AFTER_UNMUTE), text);
            }
            for(auto const *text : settings.fadeout_to_names) {
                ComboBox_AddString(Ctrl(IDC_COMBO_FADEOUT_TO_MUTE), text);
                ComboBox_AddString(Ctrl(IDC_COMBO_FADEOUT_TO_UNMUTE), text);
            }
            for(auto const *text : settings.fadeout_speed_names) {
                ComboBox_AddString(Ctrl(IDC_COMBO_FADEOUT_SPEED_MUTE), text);
                ComboBox_AddString(Ctrl(IDC_COMBO_FADEOUT_SPEED_UNMUTE), text);
            }
            setup_option_controls();
            break;

        case WM_DESTROY:
            options_dlg = nullptr;
            break;

        case WM_COMMAND:

            switch(LOWORD(wParam)) {

            case IDOK:
                settings.run_at_startup = Button_GetCheck(Ctrl(IDC_CHECK_RUN_AT_STARTUP));
                settings.mute_overlay.enabled = Button_GetCheck(Ctrl(IDC_CHECK_OVERLAY_ENABLE_MUTE));
                settings.unmute_overlay.enabled = Button_GetCheck(Ctrl(IDC_CHECK_OVERLAY_ENABLE_UNMUTE));
                settings.mute_overlay.fadeout_time_ms = ComboBox_GetCurSel(Ctrl(IDC_COMBO_FADEOUT_AFTER_MUTE));
                settings.unmute_overlay.fadeout_time_ms = ComboBox_GetCurSel(Ctrl(IDC_COMBO_FADEOUT_AFTER_UNMUTE));
                settings.mute_overlay.fadeout_to_percent = ComboBox_GetCurSel(Ctrl(IDC_COMBO_FADEOUT_TO_MUTE));
                settings.unmute_overlay.fadeout_to_percent = ComboBox_GetCurSel(Ctrl(IDC_COMBO_FADEOUT_TO_UNMUTE));
                settings.mute_overlay.fadeout_speed_ms = ComboBox_GetCurSel(Ctrl(IDC_COMBO_FADEOUT_SPEED_MUTE));
                settings.unmute_overlay.fadeout_speed_ms = ComboBox_GetCurSel(Ctrl(IDC_COMBO_FADEOUT_SPEED_UNMUTE));
                settings.save();
                PostMessage(main_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
                EndDialog(hwndDlg, LOWORD(wParam));
                break;

            case IDCANCEL:
                EndDialog(hwndDlg, LOWORD(wParam));
                break;
            }
        }
        return FALSE;
    }
}