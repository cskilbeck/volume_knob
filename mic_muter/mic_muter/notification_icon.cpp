//////////////////////////////////////////////////////////////////////

#include "framework.h"

namespace
{
    //////////////////////////////////////////////////////////////////////

    LOG_CONTEXT("icon");

#if defined(_DEBUG)
    class __declspec(uuid("D0B47348-3429-4153-8213-298C1D4E8D26")) icon_guid;
#else
    class __declspec(uuid("D019AEF9-2868-41C6-AD3F-79023844F8E7")) icon_guid;
#endif
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::load()
    {
        HR(LoadIconMetric(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_ICON_MIC_MUTE), LIM_SMALL, &muted_icon));
        HR(LoadIconMetric(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_ICON_MIC_NORMAL), LIM_SMALL, &normal_icon));
        HR(LoadIconMetric(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_ICON_MIC_MISSING), LIM_SMALL, &missing_icon));

        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = main_hwnd;
        nid.uFlags = NIF_GUID | NIF_MESSAGE;
        nid.guidItem = __uuidof(icon_guid);
        nid.uCallbackMessage = WM_NOTIFICATION_ICON;
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

    HRESULT notification_icon::update(bool attached, bool muted)
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = main_hwnd;
        if(!attached) {
            nid.hIcon = missing_icon;
        } else if(muted) {
            nid.hIcon = muted_icon;
        } else {
            nid.hIcon = normal_icon;
        }
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
