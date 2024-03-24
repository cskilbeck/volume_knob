#pragma once

//////////////////////////////////////////////////////////////////////

void clk_init();    // CH554 clock selection and configuration

void delay_uS(uint16 N);    // Delay in uS
void delay_mS(uint16 N);    // Delay in mS

void watchdog_mode_select(uint8 mode);    // watchdog settings
void watchdog_feed(uint8 count);          // feed the watchdog

#if defined(DEBUG)

void uart0_alternate_pins();
void uart0_init();
int putchar(int c);
void hexdump(char __code const *msg, uint8 *p, uint8 n);

#else

#define uart0_alternate_pins() NOP_MACRO
#define uart0_init() NOP_MACRO
#define putchar(c) NOP_MACRO
#define hexdump(m, p, n) NOP_MACRO

#endif
