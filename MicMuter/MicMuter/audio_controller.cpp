// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "framework.h"

namespace chs
{
    // ----------------------------------------------------------------------

    audio_controller::audio_controller() : endpoint_registered(false), volume_registered(false), m_cRef(1)
    {
    }

    // ----------------------------------------------------------------------

    audio_controller::~audio_controller()
    {
    }

    // ----------------------------------------------------------------------
    //  Call when the app is done with this object before calling release.
    //  This detaches from the endpoint and releases all audio service references.
    // ----------------------------------------------------------------------

    void audio_controller::Dispose()
    {
        detach_from_endpoint();

        if(endpoint_registered) {
            enumerator->UnregisterEndpointNotificationCallback(this);
            endpoint_registered = false;
        }
        enumerator.Reset();
        audio_endpoint.Reset();
        volume_control.Reset();
    }

    // ----------------------------------------------------------------------
    //  Initialize this object.  Call after constructor.
    // ----------------------------------------------------------------------

    HRESULT audio_controller::init()
    {
        HR(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                            (LPVOID *)enumerator.GetAddressOf()));
        HR(enumerator->RegisterEndpointNotificationCallback(this));
        HR(attach_to_default_endpoint());
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Called from the UI thread when the volume is changed (see OSD.cpp
    //  WM_VOLUMECHANGE handler)
    // ----------------------------------------------------------------------

    HRESULT audio_controller::get_level_info(VOLUME_INFO *pInfo)
    {
        std::lock_guard lock(endpoint_mutex);

        if(volume_control == nullptr) {
            return E_FAIL;
        }

        HR(volume_control->GetMute(&pInfo->bMuted));
        HR(volume_control->GetVolumeStepInfo(&pInfo->nStep, &pInfo->cSteps));
        LOG_DEBUG("Microphone is {}muted", pInfo->bMuted ? "" : "not ");
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Toggle mute status of microphone
    // ----------------------------------------------------------------------

    HRESULT audio_controller::toggle_mute()
    {
        std::lock_guard lock(endpoint_mutex);

        if(volume_control == nullptr) {
            return E_FAIL;
        }

        BOOL current_mute_state;
        HR(volume_control->GetMute(&current_mute_state));
        HR(volume_control->SetMute(!current_mute_state, nullptr));
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Start monitoring the current default input audio device
    // ----------------------------------------------------------------------

    HRESULT audio_controller::attach_to_default_endpoint()
    {
        std::lock_guard lock(endpoint_mutex);

        HR(enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &audio_endpoint));
        HR(audio_endpoint->Activate(__uuidof(volume_control), CLSCTX_INPROC_SERVER, NULL, (void **)&volume_control));
        HR(volume_control->RegisterControlChangeNotify(this));
        volume_registered = true;
        LOG_DEBUG("Microphone is attached and enabled");
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Stop monitoring the device and release all associated references
    // ----------------------------------------------------------------------

    void audio_controller::detach_from_endpoint()
    {
        std::lock_guard lock(endpoint_mutex);

        if(volume_control != NULL) {
            if(volume_registered) {
                volume_control->UnregisterControlChangeNotify(this);
                volume_registered = false;
            }
            volume_control.Reset();
        }
        LOG_DEBUG("Detached from microphone");
        audio_endpoint.Reset();
    }

    // ----------------------------------------------------------------------
    //  Call this from the UI thread when the default device changes
    // ----------------------------------------------------------------------

    void audio_controller::change_endpoint()
    {
        detach_from_endpoint();
        attach_to_default_endpoint();
    }

    // ----------------------------------------------------------------------
    //  Implementation of IMMNotificationClient::OnDefaultDeviceChanged
    //
    //  When the user changes the default output device we want to stop monitoring the
    //  former default and start monitoring the new default
    // ----------------------------------------------------------------------

    HRESULT audio_controller::OnDefaultDeviceChanged(EDataFlow flow, ERole, LPCWSTR)
    {
        if(flow == eCapture && main_hwnd != nullptr) {
            PostMessage(main_hwnd, WM_ENDPOINTCHANGE, 0, 0);
        }
        // return value of this callback is ignored
        return S_OK;
    }


    // ----------------------------------------------------------------------
    //  Implementation of IAudioEndpointVolumeCallback::OnNotify
    //
    //  This is called by the audio core when anyone in any process changes the volume or
    //  mute state for the endpoint we are monitoring
    // ----------------------------------------------------------------------

    HRESULT audio_controller::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA /*pNotify*/)
    {
        if(main_hwnd != nullptr) {
            PostMessage(main_hwnd, WM_VOLUMECHANGE, 0, 0);
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------

    HRESULT audio_controller::QueryInterface(REFIID iid, void **ppUnk)
    {
        if((iid == __uuidof(IUnknown)) || (iid == __uuidof(IMMNotificationClient))) {
            *ppUnk = static_cast<IMMNotificationClient *>(this);
        } else if(iid == __uuidof(IAudioEndpointVolumeCallback)) {
            *ppUnk = static_cast<IAudioEndpointVolumeCallback *>(this);
        } else {
            *ppUnk = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    // ----------------------------------------------------------------------

    ULONG audio_controller::AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    // ----------------------------------------------------------------------

    ULONG audio_controller::Release()
    {
        long lRef = InterlockedDecrement(&m_cRef);
        if(lRef == 0) {
            delete this;
        }
        return lRef;
    }
}
