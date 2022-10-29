#pragma once

namespace chs::mic_muter
{
    extern HWND options_dlg;

    INT_PTR CALLBACK options_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
