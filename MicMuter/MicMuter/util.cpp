//////////////////////////////////////////////////////////////////////
// enable vt codes in windows console

#include "framework.h"

namespace
{
    using chs::ComPtr;
    using chs::uint32;

    LOG_CONTEXT("util");

    //////////////////////////////////////////////////////////////////////
    // wic factory admin

    INIT_ONCE init_wic_once = INIT_ONCE_STATIC_INIT;
    IWICImagingFactory *wic_factory = nullptr;

    BOOL WINAPI init_wic_factory(PINIT_ONCE, PVOID, PVOID *ifactory) noexcept
    {
        IWICImagingFactory **factory = reinterpret_cast<IWICImagingFactory **>(ifactory);
        return SUCCEEDED(
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                             IID_PPV_ARGS(factory)));    // NOLINT(clang-diagnostic-language-extension-token)
    }

    //////////////////////////////////////////////////////////////////////

    IWICImagingFactory *get_wic() noexcept
    {
        if(!InitOnceExecuteOnce(&init_wic_once, init_wic_factory, nullptr, reinterpret_cast<LPVOID *>(&wic_factory))) {

            return nullptr;
        }
        return wic_factory;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT wic_bits_per_pixel(REFGUID target_guid, uint32 *bits)
    {
        if(bits == nullptr) {
            return E_INVALIDARG;
        }

        IWICImagingFactory *wic = get_wic();
        if(wic == nullptr) {
            return E_NOINTERFACE;
        }

        ComPtr<IWICComponentInfo> cinfo;
        HR(wic->CreateComponentInfo(target_guid, cinfo.GetAddressOf()));

        WICComponentType type;
        HR(cinfo->GetComponentType(&type));

        if(type != WICPixelFormat) {
            return E_INVALIDARG;
        }

        ComPtr<IWICPixelFormatInfo> pfinfo;
        HR(cinfo.As(&pfinfo));

        HR(pfinfo->GetBitsPerPixel(bits));

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT load_image(std::vector<byte> const &src, uint32 const max_width, uint32 const max_height, HBITMAP *bmp)
    {
        if(src.empty()) {
            return E_INVALIDARG;
        }

        if(src.size() > UINT32_MAX) {
            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
        }

        auto const wic = get_wic();

        if(wic == nullptr) {
            return E_NOINTERFACE;
        }

        ComPtr<IWICStream> stream;
        HR(wic->CreateStream(&stream));

        HR(stream->InitializeFromMemory(const_cast<byte *>(src.data()), static_cast<DWORD>(src.size())));

        ComPtr<IWICBitmapDecoder> decoder;
        WICDecodeOptions constexpr options = WICDecodeMetadataCacheOnDemand;
        HR(wic->CreateDecoderFromStream(stream.Get(), nullptr, options, &decoder));

        ComPtr<IWICBitmapFrameDecode> frame;
        HR(decoder->GetFrame(0, &frame));

        ComPtr<IWICBitmapSource> bmp_src = frame.Detach();

        uint32 bmp_w;
        uint32 bmp_h;

        HR(bmp_src->GetSize(&bmp_w, &bmp_h));

        LOG_DEBUG("Bitmap is {}x{}", bmp_w, bmp_h);

        if(bmp_w > max_width || bmp_h > max_height) {

            float x = 1.0f;
            float y = 1.0f;
            if(max_width != 0) {
                x = static_cast<float>(max_width) / static_cast<float>(bmp_w);
            }
            if(max_height != 0) {
                y = static_cast<float>(max_height) / static_cast<float>(bmp_h);
            }

            float const scale = std::min(x, y);
            if(scale != 1.0f) {
                bmp_w = static_cast<uint32>(static_cast<float>(bmp_w) * scale);
                bmp_h = static_cast<uint32>(static_cast<float>(bmp_h) * scale);

                ComPtr<IWICBitmapScaler> scaler;
                HR(wic->CreateBitmapScaler(&scaler));

                WICBitmapInterpolationMode constexpr interp_mode = WICBitmapInterpolationModeHighQualityCubic;
                HR(scaler->Initialize(bmp_src.Get(), bmp_w, bmp_h, interp_mode));

                bmp_src.Attach(scaler.Detach());
            }
        }

        // Force 32 bpp BGRA dest format
        WICPixelFormatGUID const dst_format = GUID_WICPixelFormat32bppBGRA;
        WICPixelFormatGUID src_format;
        HR(bmp_src->GetPixelFormat(&src_format));

        if(src_format != dst_format) {

            ComPtr<IWICFormatConverter> fmt_converter;
            HR(wic->CreateFormatConverter(&fmt_converter));

            // some formats have > 4 channels, in which case we're out of luck

            BOOL can_convert = FALSE;
            HR(fmt_converter->CanConvert(src_format, dst_format, &can_convert));
            if(!can_convert) {
                return HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
            }

            WICBitmapDitherType constexpr dither = WICBitmapDitherTypeNone;
            WICBitmapPaletteType constexpr palette = WICBitmapPaletteTypeMedianCut;
            HR(fmt_converter->Initialize(bmp_src.Get(), dst_format, dither, nullptr, 0, palette));

            bmp_src.Attach(fmt_converter.Detach());
        }

        WICPixelFormatGUID final_format;
        HR(bmp_src->GetPixelFormat(&final_format));

        uint32 bpp;
        HR(wic_bits_per_pixel(final_format, &bpp));
        LOG_DEBUG("BPP: {}", bpp);

        size_t const pitch = (bmp_w * bpp + 7u) / 8u;
        size_t const image_size = pitch * bmp_h;

        WICRect w;
        w.X = 0;
        w.Y = 0;
        w.Width = static_cast<int>(bmp_w);
        w.Height = static_cast<int>(bmp_h);
        std::vector<byte> buffer(image_size);
        HR(bmp_src->CopyPixels(&w, static_cast<uint32>(pitch), static_cast<uint32>(image_size), buffer.data()));

        *bmp = CreateBitmap(bmp_w, bmp_h, 1, 32, buffer.data());

        return S_OK;
    }
}

namespace chs::util
{
    //////////////////////////////////////////////////////////////////////

    HRESULT load_resource_binary(std::vector<byte> &buffer, uintptr_t const id, LPSTR const type /* = RT_RCDATA */)
    {
        LOG_CONTEXT("load_resource_binary");

        HMODULE const module = GetModuleHandle(nullptr);

        HRSRC const resource = FindResource(module, reinterpret_cast<LPSTR>(id), type);
        if(resource == nullptr) {
            return WIN32_ERROR("FindResource");
        }

        HGLOBAL const mem = LoadResource(module, resource);
        if(mem == nullptr) {
            return WIN32_ERROR("LoadResource");
        }
        DEFER(FreeResource(mem));

        DWORD const size = SizeofResource(module, resource);
        if(size == 0) {
            return WIN32_ERROR("SizeofResource");
        }

        LOG_DEBUG("Resource {} is {} bytes", id, size);

        auto ptr = static_cast<byte *>(LockResource(mem));
        if(ptr == nullptr) {
            return WIN32_ERROR("LockResource");
        }
        DEFER((void)UnlockResource(ptr));    // NOLINT(clang-diagnostic-unused-value)

        buffer.assign(ptr, ptr + size);

        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    bool console_set_ansi_enabled(bool const enabled)
    {
        // ReSharper disable once CppInconsistentNaming
        int constexpr ENABLE_VT_PROCESSING = 0x0004;

        HANDLE const hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if(hConsole == INVALID_HANDLE_VALUE) {
            return false;
        }
        DWORD consoleMode;
        if(!GetConsoleMode(hConsole, &consoleMode)) {
            return false;
        }
        if(enabled) {
            consoleMode |= ENABLE_VT_PROCESSING;
        } else {
            consoleMode &= ~ENABLE_VT_PROCESSING;
        }
        return SetConsoleMode(hConsole, consoleMode);
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT load_bitmap(uintptr_t const id, HBITMAP *bmp, uint32 max_width, uint32 max_height)
    {
        LOG_CONTEXT("load_bitmap");

        if(bmp == nullptr) {
            return E_INVALIDARG;
        }
        LOG_DEBUG("Loading bitmap {}", id);
        std::vector<byte> resource_buffer;
        HR(load_resource_binary(resource_buffer, id));
        HR(load_image(resource_buffer, max_width, max_height, bmp));
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    std::string windows_error_text(DWORD const err)
    {
        char *buffer = nullptr;

        DWORD constexpr flags =
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;

        DWORD len = FormatMessage(flags, nullptr, err, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

        // trim trailing whitespace
        while(len != 0 && isspace(buffer[len - 1])) {
            len -= 1;
        }

        std::string s(buffer, len);

        LocalFree(buffer);

        return s;
    }
}
