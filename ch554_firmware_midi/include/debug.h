//////////////////////////////////////////////////////////////////////

#pragma once

#pragma disable_warning 110

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

//////////////////////////////////////////////////////////////////////

#ifndef UART0_BUAD
#define UART0_BUAD 115200
#endif

#ifndef UART1_BAUD
#define UART1_BUAD 115200
#endif

//////////////////////////////////////////////////////////////////////

void CfgFsys();    // CH554 clock selection and configuration

void mDelayuS(uint16_t N);    // Delay in uS
void mDelaymS(uint16_t N);    // Delay in mS

void UART0_Alternate_Pins();              // serial port 0 pins to P0.2/P0.3
void UART0_Init();                        // T1 as a baud rate generator
uint8_t UART0_Get_Byte();                 // UART0 query method receives a byte
void UART0_Send_Byte(uint8_t Senddat);    // uart0 Send a byte

void UART1_Init();                        //
uint8_t UART1_Get_Byte();                 // UART1 query method receives a byte
void UART1_Send_Byte(uint8_t Senddat);    // uart1 Send a byte

void WDT_Mode_Select(uint8_t Mode);    // watchdog settings
void WDT_Feed(uint8_t TIM);            //  feed the watchdog

void putnibble(uint8 n);
void putstr(char *p);
void puthex(uint8 b);
void hexdump(char *msg, uint8 *p, uint8 n);
