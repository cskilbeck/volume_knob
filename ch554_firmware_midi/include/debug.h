// printf and delay

#pragma once

#include <stdint.h>

#ifndef UART0_BUAD
#define UART0_BUAD 9600
#endif

#ifndef UART1_BAUD
#define UART1_BUAD 9600
#endif

void CfgFsys();    // CH554 clock selection and configuration

void mDelayuS(uint16_t N);    // Us delay in uS
void mDelaymS(uint16_t N);    // Delay in mS

void CH554UART0Alter();                      // CH554 serial port 0 pins to P0.2/P0.3
void init_uart0();                           // T1 as a baud rate generator
uint8_t CH554UART0RcvByte();                 // CH554 UART0 query method receives a byte
void CH554UART0SendByte(uint8_t Senddat);    // Ch554uart0 Send a byte

void UART1Setup();                           //
uint8_t CH554UART1RcvByte();                 // CH554 UART1 query method receives a byte
void CH554UART1SendByte(uint8_t Senddat);    // Ch554uart1 Send a byte

// void putchar(char data);
// char getchar();

void CH554WDTModeSelect(uint8_t Mode);    // CH554 watchdog settings
void CH554WDTFeed(uint8_t TIM);           // CH554 feed the watchdog
