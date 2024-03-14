#pragma once

//////////////////////////////////////////////////////////////////////

void clk_init();    // CH554 clock selection and configuration

void delay_uS(uint16_t N);    // Delay in uS
void delay_mS(uint16_t N);    // Delay in mS

void watchdog_mode_select(uint8_t Mode);    // watchdog settings
void watchdog_feed(uint8_t TIM);            // feed the watchdog

#if defined(DEBUG)

void uart0_alternate_pins();
void uart0_init();
void putchar(char c);
void putnibble(uint8 n);
void putstr(char *p);
void puthex(uint8 b);
void hexdump(char *msg, uint8 *p, uint8 n);
void print_uint32(char *msg, uint32 v);
void print_uint16(char *msg, uint16 v);
void print_uint8(char *msg, uint8 v);

#else

#define NOP_MACRO \
    do {          \
    } while(false)

#define uart0_alternate_pins() NOP_MACRO
#define uart0_init() NOP_MACRO
#define putchar(c) NOP_MACRO
#define putnibble(n) NOP_MACRO
#define putstr(p) NOP_MACRO
#define puthex(b) NOP_MACRO
#define hexdump(m, p, n) NOP_MACRO
#define print_uint32(m, v) NOP_MACRO
#define print_uint16(m, v) NOP_MACRO
#define print_uint8(m, v) NOP_MACRO

#endif
