#pragma once

// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace chs
{
    struct VOLUME_INFO
    {
        UINT nStep;
        UINT cSteps;
        BOOL bMuted;
    };

    class audio_controller : IMMNotificationClient, IAudioEndpointVolumeCallback
    {
        LOG_CONTEXT("audio");

    private:
        bool endpoint_registered;
        bool volume_registered;
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> audio_endpoint;
        ComPtr<IAudioEndpointVolume> volume_control;
        std::mutex endpoint_mutex;

        long m_cRef;

        ~audio_controller();    // refcounted object... make the destructor private
        HRESULT attach_to_default_endpoint();
        void detach_from_endpoint();


        // IMMNotificationClient (only need to really implement OnDefaultDeviceChanged)
        IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR /*pwstrDeviceId*/, DWORD /*dwNewState*/)
        {
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceAdded(LPCWSTR /*pwstrDeviceId*/)
        {
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceRemoved(LPCWSTR /*pwstrDeviceId*/)
        {
            return S_OK;
        }

        IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId);    // ****

        IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR /*pwstrDeviceId*/, const PROPERTYKEY /*key*/)
        {
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceQueryRemove()
        {
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceQueryRemoveFailed()
        {
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceRemovePending()
        {
            return S_OK;
        }

        IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);

        IFACEMETHODIMP QueryInterface(const IID &iid, void **ppUnk);

    public:
        audio_controller();

        HRESULT init();
        void Dispose();
        HRESULT get_level_info(VOLUME_INFO *pInfo);
        void change_endpoint();

        HRESULT toggle_mute();

        // IUnknown
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
    };
}