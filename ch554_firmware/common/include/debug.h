#pragma once

//////////////////////////////////////////////////////////////////////

void clk_init();    // CH554 clock selection and configuration

void delay_uS(uint16 N);    // Delay in uS
void delay_mS(uint16 N);    // Delay in mS

void watchdog_mode_select(uint8 mode);    // watchdog settings
void watchdog_feed(uint8 count);          // feed the watchdog

void nano_putchar(uint8_t c);

#if defined(DEBUG)

void uart0_alternate_pins();
void uart0_init();
void nano_printf(char const *fmt, ...) __reentrant;
void puts(char const *line);
void hexdump(char __code const *msg, uint8 *p, uint8 n);

#define printf nano_printf

#else

#define uart0_alternate_pins() NOP_MACRO
#define uart0_init() NOP_MACRO
#define hexdump(m, p, n) NOP_MACRO

#endif
