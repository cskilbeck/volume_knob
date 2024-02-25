/********************************** (C) COPYRIGHT *******************************
 * File Name          : Debug.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2017/01/20
 * Description        : CH554 DEBUG Interface
 *******************************************************************************/

#include <stdint.h>

#include "ch554.h"
#include "debug.h"

/*******************************************************************************
* Function Name  : CfgFsys( )
* Description: ch554 clock selection and configuration function, use fsys 6MHz by default, FREQ_SYS can pass
                   CLOCK_CFG configuration is obtained, the formula is as follows:
                   Fsys = (FOSC * 4/(CLOCK_CFG & Mask_Sys_CK_SEL); specific clock needs to configure it by yourself
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CfgFsys()
{
    // 		SAFE_MOD = 0x55;
    // 		SAFE_MOD = 0xAA;
    //     CLOCK_CFG |= bOSC_EN_XT;                          //Make external crystal
    //     CLOCK_CFG &= ~bOSC_EN_INT;                        //Turn off the internal crystal

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

#if FREQ_SYS == 32000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x07;    // 32MHz
#elif FREQ_SYS == 24000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x06;    // 24MHz
#elif FREQ_SYS == 16000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x05;    // 16MHz
#elif FREQ_SYS == 12000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x04;    // 12MHz
#elif FREQ_SYS == 6000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x03;    // 6MHz
#elif FREQ_SYS == 3000000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x02;    // 3MHz
#elif FREQ_SYS == 750000
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x01;    // 750KHz
#elif FREQ_SYS == 187500
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x00;    // 187.5KHz
#else
#warning FREQ_SYS invalid or not set
#endif

    SAFE_MOD = 0x00;
}

/*******************************************************************************
 * Function Name  : mDelayus(UNIT16 n)
 * Description    : uS Delay function
 * Input          : UNIT16 n
 * Output         : None
 * Return         : None
 *******************************************************************************/
void mDelayuS(uint16_t n)    // Delay in uS
{
#ifdef FREQ_SYS
#if FREQ_SYS <= 6000000
    n >>= 2;
#endif
#if FREQ_SYS <= 3000000
    n >>= 2;
#endif
#if FREQ_SYS <= 750000
    n >>= 4;
#endif
#endif
    while(n) {         // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
        ++SAFE_MOD;    // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef FREQ_SYS
#if FREQ_SYS >= 14000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 16000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 18000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 20000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 22000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 24000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 26000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 28000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 30000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 32000000
        ++SAFE_MOD;
#endif
#endif
        --n;
    }
}

/*******************************************************************************
 * Function Name  : mDelaymS
 * Description    : Delay in mS
 * Input          : uint16_t n
 * Output         : None
 * Return         : None
 *******************************************************************************/
void mDelaymS(uint16_t n)    // Delay in the unit of MS
{
    while(n) {
#ifdef DELAY_MS_HW
        while((TKEY_CTRL & bTKC_IF) == 0)
            ;
        while(TKEY_CTRL & bTKC_IF)
            ;
#else
        mDelayuS(1000);
#endif
        --n;
    }
}

/*******************************************************************************
 * Function Name  : UART0_Alternate_Pins()
 * Description    : Serial 0 pin mapping, serial port mapping to P0.2 and P0.3
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART0_Alternate_Pins()
{
    PIN_FUNC |= bUART0_PIN_X;    // Map in serial port to P1.2 and P1.3
}

/*******************************************************************************
 * Function Name  : UART0_Init()
 * Description    : serial 0 initialization, the baud rate generator used by T1 as UART0 by default, you can also use T2
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART0_Init()
{
    SM0 = 0;
    SM1 = 1;
    SM2 = 0;     // Serial 0 use mode 1, use Timer1
    RCLK = 0;    // UART0 Receive the clock
    TCLK = 0;    // UART0 Send the clock
    PCON |= SMOD;

    // volatile uint32_t x;
    // volatile uint8_t x2;
    // x = 10 * FREQ_SYS / UART0_BUAD / 16;    // If you change the main frequency, pay attention not to overflow the value of x
    // x2 = x % 10;
    // x /= 10;
    // if(x2 >= 5)
    //     x++;    // rounding

#define _BAUD_X1(clk, baud) (10 * clk / baud / 16)
#define _BAUD_X2(clk, baud) (_BAUD_X1(clk, baud) % 10)
#define _BAUD_X3(clk, baud) (_BAUD_X2(clk, baud) / 5)
#define _BAUD_X4(clk, baud) ((_BAUD_X1(clk, baud) / 10) + _BAUD_X3(clk, baud))
#define BAUD_SET(clk, baud) ((uint8)(0x100 - _BAUD_X4(clk, baud)))

#define BAUD_REG BAUD_SET(FREQ_SYS, 115200)

    TMOD = TMOD & ~bT1_GATE & ~bT1_CT & ~MASK_T1_MOD | bT1_M1;    // Timer 1 8-bit automatic load
    T2MOD = T2MOD | bTMR_CLK | bT1_CLK;                           // Timer 1 clock select
    TH1 = BAUD_REG;
    TI = 1;     // Set Transmit IRQ so we don't wait indefinitely before sending 1st byte
    TR1 = 1;    // Start Timer 1
    REN = 1;    // Serial 0 receive enable
}

/*******************************************************************************
 * Function Name  : UART0_Get_Byte()
 * Description    : UART0 Receive a byte
 * Input          : None
 * Output         : None
 * Return         : SBUF
 *******************************************************************************/
uint8_t UART0_Get_Byte()
{
    while(RI == 0) {
    }
    RI = 0;
    return SBUF;
}

/*******************************************************************************
 * Function Name  : UART0_Send_Byte(uint8_t SendDat)
 * Description    : UART0 Send a byte
 * Input          : uint8_t SendDat；Data to be sent
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART0_Send_Byte(uint8_t SendDat)
{
    SBUF = SendDat;
    while(TI == 0) {
    }
    TI = 0;
}

//////////////////////////////////////////////////////////////////////

void putchar(char c)
{
    while(!TI) {
    }
    TI = 0;
    SBUF = c;
}

//////////////////////////////////////////////////////////////////////

char getchar()
{
    while(!RI) {
    }
    RI = 0;
    return SBUF;
}

/*******************************************************************************
 * Function Name  : UART1_Init()
 * Description    : Serial 1 initialization
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/

void UART1_Init()
{
    U1SM0 = 0;     // UART1 Select 8 -bit data bit
    U1SMOD = 1;    // Fast mode
    U1REN = 1;     // Be able to receive
    SBAUD1 = (uint8_t)(256 - FREQ_SYS / 16 / UART1_BUAD);
}

/*******************************************************************************
 * Function Name  : UART1_Get_Byte()
 * Description    : UART1 Receive a byte
 * Input          : None
 * Output         : None
 * Return         : SBUF
 *******************************************************************************/
uint8_t UART1_Get_Byte()
{
    while(U1RI == 0)
        ;    // Inquiry and receiving, the interrupt method is not used
    U1RI = 0;
    return SBUF1;
}

/*******************************************************************************
 * Function Name  : UART1_Send_Byte(uint8_t SendDat)
 * Description    : UART1 Send a byte
 * Input          : uint8_t Senddat; data to be sent
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART1_Send_Byte(uint8_t SendDat)
{
    SBUF1 = SendDat;    // Inquiry sending, the interrupt method does not require the following 2 statements, but before sending TI=0
    while(U1TI == 0)
        ;
    U1TI = 1;
}

/*******************************************************************************
* Function Name  : WDT_Mode_Select(uint8_t mode)
* Description    : Watch Dog mode selection
* Input          : uint8_t mode
                   0  timer
                   1  watchDog
* Output         : None
* Return         : None
*******************************************************************************/
void WDT_Mode_Select(uint8_t mode)
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xaa;    // Entering security mode
    if(mode) {
        GLOBAL_CFG |= bWDOG_EN;    // Start watch the door dog reset
    } else
        GLOBAL_CFG &= ~bWDOG_EN;    // Start watching the door dog as a timer
    SAFE_MOD = 0x00;                // exit safe mode
    WDOG_COUNT = 0;                 // Watch the door dog to give the initial value
}

/*******************************************************************************
* Function Name  : WDT_Feed(uint8_t tim)
* Description    : Set time to watch the door
* Input          : uint8_t tim Settings of the Dog resetting time
                   00H(6MHz)=2.8s
                   80H(6MHz)=1.4s
* Output         : None
* Return         : None
*******************************************************************************/
void WDT_Feed(uint8_t tim)
{
    WDOG_COUNT = tim;    // See the door dog counter assignment
}


//////////////////////////////////////////////////////////////////////

void putnibble(uint8 n)
{
    if(n > 9) {
        n += 'A' - 10;
    } else {
        n += '0';
    }
    putchar(n);
}

//////////////////////////////////////////////////////////////////////

void putstr(char *p)
{
    while(*p != 0) {
        putchar(*p++);
    }
}

//////////////////////////////////////////////////////////////////////

void puthex(uint8 b)
{
    putnibble(b >> 4);
    putnibble(b & 0xf);
}

//////////////////////////////////////////////////////////////////////

void hexdump(char *msg, uint8 *p, uint8 n)
{
    putstr(msg);
    putchar(':');
    while(n-- != 0) {
        puthex(*p++);
    }
    putchar('\n');
}
