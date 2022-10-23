#pragma once

namespace chs
{
    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;

    template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    namespace util
    {
        HRESULT load_resource_binary(std::vector<byte> &buffer, uintptr_t const id, LPSTR const type = RT_RCDATA);
        HRESULT load_bitmap(uintptr_t const id, HBITMAP *bmp, uint32 max_width = 0, uint32 max_height = 0);
        bool console_set_ansi_enabled(bool const enabled);
        std::string windows_error_text(DWORD err);

        //////////////////////////////////////////////////////////////////////
        // DEFER

        namespace defer
        {
            template <typename F> class defer_finalizer
            {
                F function;
                bool cancelled;

            public:
                // dummy bool parameter to disambiguate from the copy ctor

                template <typename T>
                explicit defer_finalizer(T &&f_, bool const) : function(std::forward<T>(f_)), cancelled(false)
                {
                }

                defer_finalizer(const defer_finalizer &) = delete;

                defer_finalizer(defer_finalizer &&other) noexcept
                    : function(std::move(other.function)), cancelled(other.cancelled)
                {
                    other.cancelled = true;
                }

                void cancel()
                {
                    cancelled = true;
                }

                ~defer_finalizer()
                {
                    if(!cancelled) {
                        function();
                    }
                }
            };

            template <typename F> defer_finalizer<F> deferred(F &&f)
            {
                return defer_finalizer<F>(std::forward<F>(f), true);
            }

            inline struct deferrer_struct
            {
                template <typename F> defer_finalizer<F> operator<<(F &&f)
                {
                    return defer_finalizer<F>(std::forward<F>(f), true);
                }
            } deferrer;

        }    // namespace defer
    }
}

//////////////////////////////////////////////////////////////////////

#define MACRO_JOIN2(x, y) x##y
#define MACRO_JOIN(x, y) MACRO_JOIN2(x, y)

#define DEFERRED chs::util::defer::deferrer <<

#define SCOPED auto MACRO_JOIN(deferred_lambda_call, __COUNTER__) = chs::util::defer::deferrer <<

#define DEFER(X) \
    SCOPED[=]    \
    {            \
        X;       \
    }

//////////////////////////////////////////////////////////////////////

#define HR(x)                                                                                             \
    do {                                                                                                  \
        HRESULT __hr = (x);                                                                               \
        if(FAILED(__hr)) {                                                                                \
            LOG_ERROR("{} failed: 0x{:08x} {}", #x, (uint32)__hr, ::chs::util::windows_error_text(__hr)); \
            return __hr;                                                                                  \
        }                                                                                                 \
    } while(false)

#define WIN32_ERROR(x)                                                                     \
    [&]() {                                                                                \
        DWORD _err = GetLastError();                                                       \
        LOG_ERROR("{} failed: {:08x} {}", x, _err, ::chs::util::windows_error_text(_err)); \
        return HRESULT_FROM_WIN32(_err);                                                   \
    }()
