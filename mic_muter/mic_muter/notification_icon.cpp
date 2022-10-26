//////////////////////////////////////////////////////////////////////

#include "framework.h"

namespace
{
    //////////////////////////////////////////////////////////////////////

    LOG_CONTEXT("icon");

#if defined(_DEBUG)
    class __declspec(uuid("75F258D7-14E5-433B-B5D7-7ECA6AABE078")) icon_guid;
#else
    class __declspec(uuid("0E4A1539-9A8A-4299-94EE-E6B9C30671CB")) icon_guid;
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
