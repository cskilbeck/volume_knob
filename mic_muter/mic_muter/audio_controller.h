#pragma once

namespace chs::mic_muter
{
    class audio_controller : IMMNotificationClient, IAudioEndpointVolumeCallback
    {
        LOG_CONTEXT("audio");

        bool endpoint_registered{ false };
        bool volume_registered{ false };
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> audio_endpoint;
        ComPtr<IAudioEndpointVolume> volume_control;
        std::mutex endpoint_mutex;

        long ref_count{ 1 };

        ~audio_controller() = default;    // refcounted object... make the destructor private

        HRESULT attach_to_default_endpoint();
        void detach_from_endpoint();

        IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
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

        IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
        IFACEMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId);
        IFACEMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId);
        IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId);
        IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);
        IFACEMETHODIMP QueryInterface(REFIID iid, void **ppUnk);

    public:
        audio_controller() = default;

        HRESULT init();
        void Dispose();
        void change_endpoint();

        HRESULT get_mic_info(bool *present, bool *muted);

        HRESULT toggle_mute();

        // IUnknown
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
    };
}