//////////////////////////////////////////////////////////////////////
// ansi
//
// vt ansi definitions

// ReSharper disable CppInconsistentNaming
#pragma once

//////////////////////////////////////////////////////////////////////

#if !defined(DISABLE_ANSI)

#define ansi_off "\033[0m"
#define ansi_inverse "\033[7m"
#define ansi_bold "\033[1m"
#define ansi_black "\033[30m"
#define ansi_red "\033[31m"
#define ansi_green "\033[32m"
#define ansi_yellow "\033[33m"
#define ansi_blue "\033[34m"
#define ansi_magenta "\033[35m"
#define ansi_cyan "\033[36m"
#define ansi_white "\033[37m"
#define ansi_back_black "\033[40m"
#define ansi_back_red "\033[41m"
#define ansi_back_green "\033[42m"
#define ansi_back_yellow "\033[43m"
#define ansi_back_blue "\033[44m"
#define ansi_back_magenta "\033[45m"
#define ansi_back_cyan "\033[46m"
#define ansi_back_white "\033[47m"
#define ansi_save_screen "\033[47h"
#define ansi_restore_screen "\033[47l"
#define ansi_cursor_on "\033[25h"
#define ansi_cursor_off "\033[25l"
#define ansi_clear_screen "\033[2j"
#define ansi_clear_eol "\033[K"
#define ansi_home "\033[H"
#define ansi_set_pos "\033[{:d}{:d}f"
#define ansi_get_pos "\033[6n"

#define ansi_offW L"\033[0m"
#define ansi_inverseW L"\033[7m"
#define ansi_boldW L"\033[1m"
#define ansi_blackW L"\033[30m"
#define ansi_redW L"\033[31m"
#define ansi_greenW L"\033[32m"
#define ansi_yellowW L"\033[33m"
#define ansi_blueW L"\033[34m"
#define ansi_magentaW L"\033[35m"
#define ansi_cyanW L"\033[36m"
#define ansi_whiteW L"\033[37m"
#define ansi_back_blackW L"\033[40m"
#define ansi_back_redW L"\033[41m"
#define ansi_back_greenW L"\033[42m"
#define ansi_back_yellowW L"\033[43m"
#define ansi_back_blueW L"\033[44m"
#define ansi_back_magentaW L"\033[45m"
#define ansi_back_cyanW L"\033[46m"
#define ansi_back_whiteW L"\033[47m"
#define ansi_save_screenW L"\033[47h"
#define ansi_restore_screenW L"\033[47l"
#define ansi_cursor_onW L"\033[25h"
#define ansi_cursor_offW L"\033[25l"
#define ansi_clear_screenW L"\033[2j"
#define ansi_clear_eolW L"\033[K"
#define ansi_homeW L"\033[H"
#define ansi_set_posW L"\033[{:d}{:d}f"
#define ansi_get_posW L"\033[6n"

#define ansi_offT TEXT("\033[0m")
#define ansi_inverseT TEXT("\033[7m")
#define ansi_boldT TEXT("\033[1m")
#define ansi_blackT TEXT("\033[30m")
#define ansi_redT TEXT("\033[31m")
#define ansi_greenT TEXT("\033[32m")
#define ansi_yellowT TEXT("\033[33m")
#define ansi_blueT TEXT("\033[34m")
#define ansi_magentaT TEXT("\033[35m")
#define ansi_cyanT TEXT("\033[36m")
#define ansi_whiteT TEXT("\033[37m")
#define ansi_back_blackT TEXT("\033[40m")
#define ansi_back_redT TEXT("\033[41m")
#define ansi_back_greenT TEXT("\033[42m")
#define ansi_back_yellowT TEXT("\033[43m")
#define ansi_back_blueT TEXT("\033[44m")
#define ansi_back_magentaT TEXT("\033[45m")
#define ansi_back_cyanT TEXT("\033[46m")
#define ansi_back_whiteT TEXT("\033[47m")
#define ansi_save_screenT TEXT("\033[47h")
#define ansi_restore_screenT TEXT("\033[47l")
#define ansi_cursor_onT TEXT("\033[25h")
#define ansi_cursor_offT TEXT("\033[25l")
#define ansi_clear_screenT TEXT("\033[2j")
#define ansi_clear_eolT TEXT("\033[K")
#define ansi_homeT TEXT("\033[H")
#define ansi_set_posT TEXT("\033[{:d}{:d}f")
#define ansi_get_posT TEXT("\033[6n")

//////////////////////////////////////////////////////////////////////

#else

#define ansi_off ""
#define ansi_inverse ""
#define ansi_bold ""
#define ansi_black ""
#define ansi_red ""
#define ansi_green ""
#define ansi_yellow ""
#define ansi_blue ""
#define ansi_magenta ""
#define ansi_cyan ""
#define ansi_white ""
#define ansi_back_black ""
#define ansi_back_red ""
#define ansi_back_green ""
#define ansi_back_yellow ""
#define ansi_back_blue ""
#define ansi_back_magenta ""
#define ansi_back_cyan ""
#define ansi_back_white ""
#define ansi_save_screen ""
#define ansi_restore_screen ""
#define ansi_cursor_on ""
#define ansi_cursor_off ""
#define ansi_clear_screen ""
#define ansi_clear_eol ""
#define ansi_home ""
#define ansi_set_pos ""
#define ansi_get_pos ""

#define ansi_offW
#define ansi_inverseW
#define ansi_boldW
#define ansi_blackW
#define ansi_redW
#define ansi_greenW
#define ansi_yellowW
#define ansi_blueW
#define ansi_magentaW
#define ansi_cyanW
#define ansi_whiteW
#define ansi_back_blackW
#define ansi_back_redW
#define ansi_back_greenW
#define ansi_back_yellowW
#define ansi_back_blueW
#define ansi_back_magentaW
#define ansi_back_cyanW
#define ansi_back_whiteW
#define ansi_save_screenW
#define ansi_restore_screenW
#define ansi_cursor_onW
#define ansi_cursor_offW
#define ansi_clear_screenW
#define ansi_clear_eolW
#define ansi_homeW
#define ansi_set_posW
#define ansi_get_posW

#define ansi_offT
#define ansi_inverseT
#define ansi_boldT
#define ansi_blackT
#define ansi_redT
#define ansi_greenT
#define ansi_yellowT
#define ansi_blueT
#define ansi_magentaT
#define ansi_cyanT
#define ansi_whiteT
#define ansi_back_blackT
#define ansi_back_redT
#define ansi_back_greenT
#define ansi_back_yellowT
#define ansi_back_blueT
#define ansi_back_magentaT
#define ansi_back_cyanT
#define ansi_back_whiteT
#define ansi_save_screenT
#define ansi_restore_screenT
#define ansi_cursor_onT
#define ansi_cursor_offT
#define ansi_clear_screenT
#define ansi_clear_eolT
#define ansi_homeT
#define ansi_set_posT
#define ansi_get_posT

#endif
