//////////////////////////////////////////////////////////////////////
// logger
//
// debug logging

#pragma once

//////////////////////////////////////////////////////////////////////

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_VERBOSE 4
#define LOG_LEVEL_DEBUG 5
#define LOG_LEVEL_ALL 5

// Compile-time logging limit:

#if !defined(LOG_LEVEL_MAX)
#if defined(_DEBUG)
#define LOG_LEVEL_MAX LOG_LEVEL_ALL
#else
#define LOG_LEVEL_MAX LOG_LEVEL_NONE
#endif
#endif

#define LOG_NOP \
    do {        \
    } while(false)

#if !defined(DISABLE_LOGGING)

//////////////////////////////////////////////////////////////////////

namespace chs::logger
{
    //////////////////////////////////////////////////////////////////////
    // This enum must match the #defines above

    enum class level
    {
        min = 0,

        none = 0,
        error = 1,
        warn = 2,
        info = 3,
        verbose = 4,
        debug = 5,

        max = 5
    };

    extern level log_level;

    extern bool log_to_stdout;

    extern std::mutex log_console_mutex;

    void set_level(level level);

    void ensure_console_exists();

    struct log_context
    {
        char const *context_ansi;
        wchar_t const *context_wide;
    };

    template <typename T> struct log_util
    {
        static void debug_write(T const *s) = delete;
        static void console_write(T const *s) = delete;
        static void file_write(T const *filename, T const *s) = delete;
        static constexpr T const *log_format_string = nullptr;
        static constexpr T const *log_names_ansi[] = {};
        static constexpr T const *log_names[] = {};
    };

    //////////////////////////////////////////////////////////////////////

    template <> struct log_util<char>
    {
        static void debug_write(char const *s)
        {
            OutputDebugStringA(s);
        }

        static void console_write(char const *s)
        {
            std::lock_guard<std::mutex> lock(log_console_mutex);
            ensure_console_exists();
            printf("%s", s);
        }

        static void format_time_ansi(char *time_buffer, size_t const buffer_size, struct tm const &timeinfo)
        {
            (void)strftime(time_buffer, buffer_size, "" ansi_blue "%d-%m-%Y,%H:%M:%S", &timeinfo);
        }

        static void format_time(char *time_buffer, size_t const buffer_size, struct tm const &timeinfo)
        {
            (void)strftime(time_buffer, buffer_size, "%d-%m-%Y,%H:%M:%S", &timeinfo);
        }

        template <typename... types>
        static std::string format_line(char const *time_buffer, level const level, log_context const &context,
                                       char const *msg, const types... args)
        {
            std::string message = std::vformat(msg, std::make_format_args(args...));
            char const *level_name = log_names[static_cast<int>(level)];
            static constexpr char const *log_format = "{:s} {:s} {: <12.12s} {:s}\n";
            return std::vformat(log_format,
                                std::make_format_args(time_buffer, level_name, context.context_ansi, message));
        }

        template <typename... types>
        static std::string format_line_ansi(char const *time_buffer, level const level, log_context const &context,
                                            char const *msg, const types... args)
        {
            std::string message = std::vformat(msg, std::make_format_args(args...));
            char const *level_name = log_names_ansi[static_cast<int>(level)];
            static constexpr char const *log_format_ansi =
                ansi_off "{:s} {:s}" ansi_yellow " {: <12.12s}" ansi_off " {:s}\n";
            return std::vformat(log_format_ansi,
                                std::make_format_args(time_buffer, level_name, context.context_ansi, message));
        }

        // clang-format off
	    static constexpr char const *log_names_ansi[] = {
            ansi_bold    ansi_off "none   " ansi_off,
		    ansi_bold    ansi_red "error  " ansi_off,
		    ansi_bold ansi_yellow "Warn   " ansi_off,
		    ansi_bold  ansi_white "Info   " ansi_off,
		    ansi_bold   ansi_cyan "Verbose" ansi_off,
            ansi_bold  ansi_green "Debug  " ansi_off
	    };

        static constexpr char const *log_names[] = {
            "None   ",
		    "Error  ",
		    "Warn   ",
		    "Info   ",
		    "Verbose",
            "Debug  "
	    };
        // clang-format on
    };

    //////////////////////////////////////////////////////////////////////

    template <> struct log_util<wchar_t>
    {
        static void debug_write(wchar_t const *s)
        {
            OutputDebugStringW(s);
        }

        static void console_write(wchar_t const *s)
        {
            std::lock_guard lock(log_console_mutex);
            ensure_console_exists();
            (void)wprintf_s(L"%s", s);
        }

        static void format_time_ansi(wchar_t *time_buffer, size_t const buffer_size, struct tm const &timeinfo)
        {
            (void)wcsftime(time_buffer, buffer_size, "" ansi_blueW L"%d-%m-%Y,%H:%M:%S", &timeinfo);
        }

        static void format_time(wchar_t *time_buffer, size_t const buffer_size, struct tm const &timeinfo)
        {
            (void)wcsftime(time_buffer, buffer_size, L"%d-%m-%Y,%H:%M:%S", &timeinfo);
        }

        template <typename... types>
        static std::wstring format_line_ansi(wchar_t const *time_buffer, level const level, log_context const &context,
                                             wchar_t const *msg, const types... args)
        {
            std::wstring message = std::vformat(msg, std::make_wformat_args(args...));
            wchar_t const *level_name = log_names_ansi[static_cast<int>(level)];
            static constexpr wchar_t const *log_format_ansi =
                ansi_offW L"{:s} {:s}" ansi_yellowW L" {:s}" ansi_offW L" {:s}\n";
            return std::vformat(log_format_ansi,
                                std::make_wformat_args(time_buffer, level_name, context.context_wide, message));
        }

        template <typename... types>
        static std::wstring format_line(wchar_t const *time_buffer, level const level, log_context const &context,
                                        wchar_t const *msg, const types... args)
        {
            std::wstring message = std::vformat(msg, std::make_wformat_args(args...));
            wchar_t const *level_name = LOG_Names[static_cast<int>(level)];
            static constexpr wchar_t const *log_format = L"{:s} {:s} {:s} {:s}\n";
            return std::vformat(log_format,
                                std::make_wformat_args(time_buffer, level_name, context.context_wide, message));
        }

        // clang-format off
	    static constexpr wchar_t const *log_names_ansi[] = {
            ansi_bold    ansi_offW L"none   " ansi_offW,
		    ansi_bold    ansi_redW L"error  " ansi_offW,
		    ansi_bold ansi_yellowW L"Warn   " ansi_offW,
		    ansi_bold  ansi_whiteW L"Info   " ansi_offW,
		    ansi_bold   ansi_cyanW L"Verbose" ansi_offW,
            ansi_bold  ansi_greenW L"Debug  " ansi_offW
        };

        static constexpr wchar_t const *LOG_Names[] = {
            L"None   ",
		    L"Error  ",
		    L"Warn   ",
		    L"Info   ",
		    L"Verbose",
            L"Debug  "
        };
        // clang-format on
    };

    //////////////////////////////////////////////////////////////////////

    template <typename T, size_t N> void get_time_info(T (&buffer)[N], bool with_ansi)
    {
        using L = log_util<T>;

        struct tm time_info;
        time_t t;
        time(&t);
        gmtime_s(&time_info, &t);
        if(with_ansi) {
            L::format_time_ansi(buffer, N, time_info);
        } else {
            L::format_time(buffer, N, time_info);
        }
    }

    //////////////////////////////////////////////////////////////////////

    template <typename T, class... types>
    __declspec(noinline) void emit_log_message(log_context const &context, level level, T const *msg,
                                               const types... args)
    {
        using L = log_util<T>;

        T time_buffer[80];
        get_time_info(time_buffer, log_to_stdout);

        if(log_to_stdout) {
            L::console_write(L::format_line_ansi(time_buffer, level, context, msg, args...).c_str());
        } else {
            L::debug_write(L::format_line(time_buffer, level, context, msg, args...).c_str());
        }
    }

    //////////////////////////////////////////////////////////////////////

    template <auto context, typename T, class... types>
    __declspec(noinline) void write_log_message(level level, T const *msg, const types... args)
    {
        if(level > log_level) {
            return;
        }

        level = std::clamp(level, level::min, level::max);

        emit_log_message(context, level, msg, std::forward<const types>(args)...);
    }

}    // namespace chs::logger

//////////////////////////////////////////////////////////////////////

#define LOG_EMIT_MESSAGE(_level, _msg, ...)                                                                    \
    do {                                                                                                       \
        if(chs::logger::log_level >= chs::logger::level::_level) {                                             \
            chs::logger::write_log_message<log_active_context>(chs::logger::level::_level, _msg, __VA_ARGS__); \
        }                                                                                                      \
    } while(false)


#if LOG_LEVEL_MAX >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(msg, ...) LOG_EMIT_MESSAGE(debug, msg, __VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...) LOG_NOP
#endif

#if LOG_LEVEL_MAX >= LOG_LEVEL_VERBOSE
#define LOG_VERBOSE(msg, ...) LOG_EMIT_MESSAGE(verbose, msg, __VA_ARGS__)
#else
#define LOG_VERBOSE(msg, ...) LOG_NOP
#endif

#if LOG_LEVEL_MAX >= LOG_LEVEL_INFO
#define LOG_INFO(msg, ...) LOG_EMIT_MESSAGE(info, msg, __VA_ARGS__)
#else
#define LOG_INFO(msg, ...) LOG_NOP
#endif

#if LOG_LEVEL_MAX >= LOG_LEVEL_WARN
#define LOG_WARN(msg, ...) LOG_EMIT_MESSAGE(warn, msg, __VA_ARGS__)
#else
#define LOG_WARN(msg, ...) LOG_NOP
#endif

#if LOG_LEVEL_MAX >= LOG_LEVEL_ERROR
#define LOG_ERROR(msg, ...) LOG_EMIT_MESSAGE(error, msg, __VA_ARGS__)
#else
#define LOG_ERROR(msg, ...) LOG_NOP
#endif

//////////////////////////////////////////////////////////////////////

#define LOG_JOIN2(x, y) x##y
#define LOG_JOIN(x, y) LOG_JOIN2(x, y)

#define LOG_CONTEXT(ctx)                                                                                \
    __pragma(warning(push))                                                                             \
        __pragma(warning(disable : 4459)) static constexpr chs::logger::log_context log_active_context{ \
            ctx, LOG_JOIN(L, ctx)                                                                       \
        } __pragma(warning(pop))

#define LOG_SET_LEVEL(l) chs::logger::set_level(logger::level::l)

//////////////////////////////////////////////////////////////////////

#else    // DISABLE_LOGGING

#define LOG_CONTEXT(...) LOG_NOP
#define LOG_LEVEL(...) LOG_NOP
#define LOG_SETLEVEL(...) LOG_NOP
#define LOG_DEBUG(...) LOG_NOP
#define LOG_VERBOSE(...) LOG_NOP
#define LOG_INFO(...) LOG_NOP
#define LOG_WARN(...) LOG_NOP
#define LOG_ERROR(...) LOG_NOP

//////////////////////////////////////////////////////////////////////

#endif
