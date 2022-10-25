//////////////////////////////////////////////////////////////////////

#include "framework.h"

//////////////////////////////////////////////////////////////////////

#if !defined(DISABLE_LOGGING)

namespace chs::logger
{
    level log_level = level::debug;
    bool log_to_stdout{ false };
    std::mutex log_console_mutex;

    //////////////////////////////////////////////////////////////////////

    void set_level(level const level)
    {
        log_level = std::clamp(level, level::min, level::max);
    }

    //////////////////////////////////////////////////////////////////////

    namespace
    {
        INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

        BOOL init_console(PINIT_ONCE, PVOID, PVOID *)
        {
            AllocConsole();
            FILE *fpstdin = stdin;
            FILE *fpstdout = stdout;
            FILE *fpstderr = stderr;
            (void)freopen_s(&fpstdin, "CONIN$", "r", stdin);
            (void)freopen_s(&fpstdout, "CONOUT$", "w", stdout);
            (void)freopen_s(&fpstderr, "CONOUT$", "w", stderr);
            util::console_set_ansi_enabled(true);
            return true;
        }

    }    // namespace

    //////////////////////////////////////////////////////////////////////

    void ensure_console_exists()
    {
        InitOnceExecuteOnce(&init_once, init_console, nullptr, nullptr);
    }

    //////////////////////////////////////////////////////////////////////

}    // namespace logview::logger

#endif
