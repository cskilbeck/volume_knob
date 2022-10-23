//////////////////////////////////////////////////////////////////////

#include "framework.h"

namespace
{
    //////////////////////////////////////////////////////////////////////

    LOG_CONTEXT("icon");

    class __declspec(uuid("CB4F485F-66A8-4A91-8D8F-3BEF481B6C3D")) icon_guid;
}

namespace chs
{
    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::load(bool is_muted)
    {
        HR(LoadIconMetric(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_ICON_MIC_MUTE), LIM_SMALL, &muted_icon));
        HR(LoadIconMetric(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_ICON_MIC_GREEN), LIM_SMALL, &normal_icon));

        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = main_hwnd;
        nid.uFlags = NIF_ICON | NIF_GUID | NIF_MESSAGE;
        nid.guidItem = __uuidof(icon_guid);
        nid.uCallbackMessage = WM_NOTIFICATION_ICON;
        nid.hIcon = is_muted ? muted_icon : normal_icon;
        if(!Shell_NotifyIcon(NIM_ADD, &nid)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        nid.uVersion = NOTIFYICON_VERSION_4;
        if(!Shell_NotifyIcon(NIM_SETVERSION, &nid)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::update(bool is_muted)
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = main_hwnd;
        nid.hIcon = is_muted ? muted_icon : normal_icon;
        nid.uFlags = NIF_ICON | NIF_GUID;
        nid.guidItem = __uuidof(icon_guid);
        if(!Shell_NotifyIcon(NIM_MODIFY, &nid)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::destroy()
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = main_hwnd;
        nid.uFlags = NIF_GUID;
        nid.guidItem = __uuidof(icon_guid);
        if(!Shell_NotifyIcon(NIM_DELETE, &nid)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        return S_OK;
    }
}
