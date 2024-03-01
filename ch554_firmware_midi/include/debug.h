#pragma once

//////////////////////////////////////////////////////////////////////

#ifndef UART0_BAUD
#define UART0_BAUD 115200
#endif

#ifndef UART1_BAUD
#define UART1_BUAD 115200
#endif

//////////////////////////////////////////////////////////////////////

void clk_init();    // CH554 clock selection and configuration

void delay_uS(uint16_t N);    // Delay in uS
void delay_mS(uint16_t N);    // Delay in mS

void uart0_alternate_pins();              // serial port 0 pins to P0.2/P0.3
void uart0_init();                        // T1 as a baud rate generator
uint8_t uart0_get_gyte();                 // UART0 query method receives a byte
void uart0_send_byte(uint8_t Senddat);    // uart0 Send a byte

void uart1_init();                        //
uint8_t uart1_get_gyte();                 // UART1 query method receives a byte
void uart1_send_byte(uint8_t Senddat);    // uart1 Send a byte

void watchdog_mode_select(uint8_t Mode);    // watchdog settings
void watchdog_feed(uint8_t TIM);            //  feed the watchdog

void putnibble(uint8 n);
void putstr(char *p);
void puthex(uint8 b);
void hexdump(char *msg, uint8 *p, uint8 n);

void print_uint32(char *msg, uint32 v);
void print_uint16(char *msg, uint16 v);
void print_uint8(char *msg, uint8 v);
