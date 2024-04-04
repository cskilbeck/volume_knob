// Cut down from original nanoprintf.c
// Removed support for many things including
//  %* field width from arg
//  %i for integer
//  float
//  64 bit (llu)
//  sprintf
// Maximum field width 255
// Maximum string size 255
// Mangled it to make the code size smaller
// Removed use of divide and modulo in base 10 int printing
// Generated code is just under 2K

#include <stdint.h>
#include <stdbool.h>

#if defined(VSCODE) || defined(_MSC_VER)
#define true 1
#define false 0
#define DEBUG
#if defined(VSCODE)
#include "vs_lint.h"
#endif
#include <stdarg.h>
#else
#include "main.h"
#endif

void nano_putchar(uint8_t c);

#if defined(DEBUG)

typedef enum
{
    NPF_FMT_SPEC_LEN_MOD_NONE,
    NPF_FMT_SPEC_LEN_MOD_SHORT,    // 'h'
    NPF_FMT_SPEC_LEN_MOD_CHAR,     // 'hh'
    NPF_FMT_SPEC_LEN_MOD_LONG,     // 'l'
} npf_format_spec_length_modifier_t;

typedef enum
{
    NPF_FMT_SPEC_CONV_PERCENT = 0,    // '%'
    NPF_FMT_SPEC_CONV_CHAR = 1,       // 'c'
    NPF_FMT_SPEC_CONV_STRING = 2,     // 's'

    NPF_FMT_SPEC_CONV_SIGNED_INT = 3,      // 'd'
    NPF_FMT_SPEC_CONV_HEX_INT = 4,         // 'x', 'X'
    NPF_FMT_SPEC_CONV_UNSIGNED_INT = 5,    // 'u'

    NPF_FMT_SPEC_IS_INTEGER = 3,
} npf_format_spec_conversion_t;

typedef int32_t npf_int_t;
typedef uint32_t npf_uint_t;

static char cbuf_mem[12];

// convert a 32 bit unsigned int to decimal or hex chars

static uint8_t npf_utoa(npf_uint_t n, npf_format_spec_conversion_t conv, uint8_t case_adj)
{
    char bcd_buf[5] = { 0, 0, 0, 0, 0 };
    uint8_t num_bytes;

    if(conv == NPF_FMT_SPEC_CONV_HEX_INT) {

        bcd_buf[0] = (uint8_t)(n >> 24);
        bcd_buf[1] = (uint8_t)(n >> 16);
        bcd_buf[2] = (uint8_t)(n >> 8);
        bcd_buf[3] = (uint8_t)(n >> 0);
        num_bytes = 4;

    } else {

        // double dabble uint32 to bcd: slow but uses very little code space and no divide or modulo

        for(uint8_t i = 0; i < 32; ++i) {
            for(uint8_t j = 0; j < 5; ++j) {
                uint8_t w = bcd_buf[j];
                uint8_t x = w & 0xF0;
                uint8_t y = w & 0x0F;
                if(y >= 5) {
                    y = (y + 3) & 0xf;
                }
                if(x >= 0x50) {
                    x += 0x30;
                }
                bcd_buf[j] = x | y;
            }
            uint8_t spill = 0;
            if((n & 0x80000000) != 0) {
                spill = 1;
            }
            n <<= 1;

            for(int8_t j = 4; j >= 0; --j) {

                uint8_t new_spill = ((bcd_buf[j] & 0x80) != 0) ? 1 : 0;
                bcd_buf[j] = (bcd_buf[j] << 1) | spill;
                spill = new_spill;
            }
        }
        num_bytes = 5;
    }

    uint8_t num_chars = 0;
    bool got_one = false;

    for(uint8_t i = 0; i < num_bytes; ++i) {
        uint8_t b = bcd_buf[i];
        for(uint8_t n = 0; n < 2; ++n) {
            uint8_t nibble = b >> 4;
            if(got_one || nibble != 0) {
                char c = nibble + '0';
                if(nibble > 9) {
                    c = nibble + 'A' - 10 + case_adj;
                }
                got_one = true;
                cbuf_mem[num_chars] = c;
                num_chars += 1;
            }
            b <<= 4;
        }
    }
    if(num_chars == 0) {
        cbuf_mem[0] = '0';
        num_chars = 1;
    }
    return num_chars;
}

#define NPF_PUTC(VAL) nano_putchar(VAL)

#define NPF_EXTRACT(MOD, CAST_TO, EXTRACT_AS)    \
    case NPF_FMT_SPEC_LEN_MOD_##MOD:             \
        val = (CAST_TO)va_arg(args, EXTRACT_AS); \
        break

void nano_printf(char const *format, ...)
{
    va_list args;
    va_start(args, format);

    char const *cur = format;

    npf_format_spec_length_modifier_t length_modifier = NPF_FMT_SPEC_LEN_MOD_NONE;
    npf_format_spec_conversion_t conv_spec = NPF_FMT_SPEC_CONV_PERCENT;

    while(*cur) {

        int8_t format_spec_len = 0;
        uint8_t field_width = 0;
        bool left_justified = false;
        bool leading_zero_pad = false;
        char case_adjust = 'a' - 'A';    // lowercase

        if(*cur == '%') {

            char const *format = cur;

            while(*++cur) {    // cur points at the leading '%' character

                switch(*cur) {    // Optional flags

                case '-':
                    left_justified = true;
                    leading_zero_pad = false;
                    continue;

                case '0':
                    leading_zero_pad = !left_justified;
                    continue;

                default:
                    break;
                }
                break;
            }

            field_width = 0;

            while((*cur >= '0') && (*cur <= '9')) {
                field_width = (uint8_t)(field_width * (uint8_t)10) + (*cur++ - '0');
            }

            int tmp_conv = -1;
            length_modifier = NPF_FMT_SPEC_LEN_MOD_NONE;

            switch(*cur++) {    // Length modifier

            case 'h':
                length_modifier = NPF_FMT_SPEC_LEN_MOD_SHORT;
                if(*cur == 'h') {
                    length_modifier = NPF_FMT_SPEC_LEN_MOD_CHAR;
                    ++cur;
                }
                break;

            case 'l':
                length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG;
                break;

            default:
                cur -= 1;
                break;
            }

            switch(*cur++) {    // Conversion specifier

            case '%':
                conv_spec = NPF_FMT_SPEC_CONV_PERCENT;
                break;

            case 'c':
                conv_spec = NPF_FMT_SPEC_CONV_CHAR;
                break;

            case 's':
                conv_spec = NPF_FMT_SPEC_CONV_STRING;
                leading_zero_pad = false;
                break;

            case 'd':
                tmp_conv = NPF_FMT_SPEC_CONV_SIGNED_INT;

            case 'u':
                if(tmp_conv == -1) {
                    tmp_conv = NPF_FMT_SPEC_CONV_UNSIGNED_INT;
                }

            case 'X':
                if(tmp_conv == -1) {
                    case_adjust = 0;
                }

            case 'x':
                if(tmp_conv == -1) {
                    tmp_conv = NPF_FMT_SPEC_CONV_HEX_INT;
                }
                conv_spec = (npf_format_spec_conversion_t)tmp_conv;
                break;

            default:
                format = cur;
                break;
            }
            format_spec_len = (int8_t)(cur - format);
        }

        if(format_spec_len == 0) {
            NPF_PUTC(*cur++);
            continue;
        }

        char *cbuf = cbuf_mem;
        char sign_c = 0;
        int8_t cbuf_len = 0;
        int8_t field_pad = 0;
        char pad_c = 0;

        // Extract and convert the argument to string, point cbuf at the text.

        switch(conv_spec) {

        case NPF_FMT_SPEC_CONV_PERCENT:
            *cbuf = '%';
            cbuf_len = 1;
            break;

        case NPF_FMT_SPEC_CONV_CHAR:
            *cbuf = (char)va_arg(args, int);
            cbuf_len = 1;
            break;

        case NPF_FMT_SPEC_CONV_STRING: {
            cbuf = va_arg(args, char *);
            for(char const *s = cbuf; *s != 0; ++s, ++cbuf_len)
                ;    // strlen
        } break;

        case NPF_FMT_SPEC_CONV_SIGNED_INT: {
            npf_int_t val = 0;
            switch(length_modifier) {
                NPF_EXTRACT(NONE, int, int);
                NPF_EXTRACT(SHORT, short, int);
                NPF_EXTRACT(CHAR, char, int);
                NPF_EXTRACT(LONG, long, long);
            }

            sign_c = 0;
            if(val < 0) {
                sign_c = '-';
                val = -val;
            }
            cbuf_len = npf_utoa(val, NPF_FMT_SPEC_CONV_SIGNED_INT, 0);
        } break;

        case NPF_FMT_SPEC_CONV_HEX_INT:
        case NPF_FMT_SPEC_CONV_UNSIGNED_INT: {
            npf_uint_t val = 0;

            switch(length_modifier) {
                NPF_EXTRACT(NONE, unsigned, unsigned);
                NPF_EXTRACT(SHORT, unsigned short, unsigned);
                NPF_EXTRACT(CHAR, unsigned char, unsigned);
                NPF_EXTRACT(LONG, unsigned long, unsigned long);
            }

            cbuf_len = npf_utoa(val, conv_spec, (unsigned)case_adjust);

        } break;

        default:
            break;
        }

        // Compute the field width pad character
        if(field_width != 0) {
            pad_c = ' ';
            if(leading_zero_pad && conv_spec >= NPF_FMT_SPEC_IS_INTEGER) {
                pad_c = '0';
            }
        }

        // Given the full converted length, how many pad bytes?
        field_pad = field_width - cbuf_len - !!sign_c;
        if(field_pad < 0) {
            field_pad = 0;
        }

        // Apply right-justified field width if requested
        if(!left_justified && pad_c != 0) {    // If leading zeros pad, sign goes first.
            if(pad_c == '0') {
                if(sign_c) {
                    NPF_PUTC(sign_c);
                    sign_c = 0;
                }
            }
            while(field_pad-- > 0) {
                NPF_PUTC(pad_c);
            }
        }

        // Write the converted payload
        if(sign_c) {
            NPF_PUTC(sign_c);
        }

        for(int i = 0; i < cbuf_len; ++i) {
            NPF_PUTC(cbuf[i]);
        }

        if(left_justified && pad_c) {    // Apply left-justified field width
            while(field_pad-- > 0) {
                NPF_PUTC(pad_c);
            }
        }
    }
}

// double dabble: convert 32 bit unsigned integer to bcd
// right justified, fills output with leading zeros
// no mods or divs

#undef NPF_PUTC
#undef NPF_EXTRACT

/*
  nanoprintf is dual-licensed under both the "Unlicense" and the
  "Zero-Clause BSD" (0BSD) licenses. The intent of this dual-licensing
  structure is to make nanoprintf as consumable as possible in as many
  environments / countries / companies as possible without any
  encumberances.

  The text of the two licenses follows below:

  ============================== UNLICENSE ==============================

  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org>

  ================================ 0BSD =================================

  Copyright (C) 2019- by Charles Nicholson <charles.nicholson+nanoprintf@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for
  any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#endif