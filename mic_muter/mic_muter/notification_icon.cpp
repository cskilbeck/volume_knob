//////////////////////////////////////////////////////////////////////

#include "framework.h"

namespace
{
    //////////////////////////////////////////////////////////////////////

    LOG_CONTEXT("icon");

#if defined(_DEBUG)
    class __declspec(uuid("CC2EC94D-B8E2-4FF5-9C4C-41DCD4AC8D87")) icon_guid;
#else
    class __declspec(uuid("8312E0D8-5CB0-4799-B13F-B52B8AE6F61E")) icon_guid;
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
        nid.uCallbackMessage = WM_APP_NOTIFICATION_ICON;
        if(!Shell_NotifyIcon(NIM_ADD, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_ADD)");
        }

        nid.uVersion = NOTIFYICON_VERSION_4;
        if(!Shell_NotifyIcon(NIM_SETVERSION, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_SETVERSION)");
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
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_MODIFY)");
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::destroy()
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = main_hwnd;
        nid.uFlags = NIF_GUID | NIF_MESSAGE;
        nid.uCallbackMessage = WM_APP_NOTIFICATION_ICON;
        nid.guidItem = __uuidof(icon_guid);
        if(!Shell_NotifyIcon(NIM_DELETE, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_DELETE)");
        }
        return S_OK;
    }
}
