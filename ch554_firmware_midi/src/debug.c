/********************************** (C) COPYRIGHT *******************************
* File Name          : Debug.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554 DEBUG Interface
                    CH554 main frequency modification, delay function definition
                    Candidate 0 and serial port 1 initialization
                    The transceiver function of serial port 0 and serial 1
                    Watch the door dog initialization
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
    CLOCK_CFG = CLOCK_CFG & ~MASK_SYS_CK_SEL | 0x00;    // 187.5MHz
#else
#warning FREQ_SYS invalid or not set
#endif

    SAFE_MOD = 0x00;
}



/*******************************************************************************
 * Function Name  : mDelayus(UNIT16 n)
 * Description    : us延时函数
 * Input          : UNIT16 n
 * Output         : None
 * Return         : None
 *******************************************************************************/
void mDelayuS(uint16_t n)    // 以uS为单位延时
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
 * Function Name  : mDelayms(UNIT16 n)
 * Description    : msDelay function
 * Input          : UNIT16 n
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
 * Function Name  : CH554UART0Alter()
 * Description    : CH554 Serial 0 pin mapping, serial port mapping to P0.2 and P0.3
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH554UART0Alter()
{
    PIN_FUNC |= bUART0_PIN_X;    // Map in serial port to P1.2 and P1.3
}

/*******************************************************************************
 * Function Name  : mInitSTDIO()
 * Description    : CH554 serial 0 initialization, the baud rate generator used by T1 as UART0 by default, you can also use T2
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void init_uart0()
{
    volatile uint32_t x;
    volatile uint8_t x2;

    SM0 = 0;
    SM1 = 1;
    SM2 = 0;     // Serial 0 use mode 1, use Timer1
    RCLK = 0;    // UART0 Receive the clock
    TCLK = 0;    // UART0 Send the clock
    PCON |= SMOD;
    x = 10 * FREQ_SYS / UART0_BUAD / 16;    // If you change the main frequency, pay attention not to overflow the value of x
    x2 = x % 10;
    x /= 10;
    if(x2 >= 5)
        x++;    // rounding

    TMOD = TMOD & ~bT1_GATE & ~bT1_CT & ~MASK_T1_MOD | bT1_M1;    // 0X20，Timer1 As an 8 -bit automatic load timer
    T2MOD = T2MOD | bTMR_CLK | bT1_CLK;                           // Timer1 Clock selection
    TH1 = 0 - x;                                                  // 12MHz Crystal, baud/12 For actual need to set the baud rate
    TR1 = 1;                                                      // Start the timer 1
    TI = 1;
    REN = 1;    // Serial 0 receive enable
}

/*******************************************************************************
 * Function Name  : CH554UART0RcvByte()
 * Description    : CH554UART0 Receive a byte
 * Input          : None
 * Output         : None
 * Return         : SBUF
 *******************************************************************************/
uint8_t CH554UART0RcvByte()
{
    while(RI == 0)
        ;    // Inquiry and receiving, the interrupt method is not used
    RI = 0;
    return SBUF;
}

/*******************************************************************************
 * Function Name  : CH554UART0SendByte(uint8_t SendDat)
 * Description    : CH554UART0 Send a byte
 * Input          : uint8_t SendDat；Data to be sent
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH554UART0SendByte(uint8_t SendDat)
{
    SBUF = SendDat;    // Inquiry sending, the interrupt method does not require the following 2 statements, but before sending TI=0
    while(TI == 0)
        ;
    TI = 0;
}

void putchar(char c)
{
    while(!TI) /* assumes UART is initialized */
        ;
    TI = 0;
    SBUF = c;
}

char getchar()
{
    while(!RI)
        ; /* assumes UART is initialized */
    RI = 0;
    return SBUF;
}

/*******************************************************************************
 * Function Name  : UART1Setup()
 * Description    : CH554 Serial 1 initialization
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void UART1Setup()
{
    U1SM0 = 0;     // UART1 Select 8 -bit data bit
    U1SMOD = 1;    // Fast mode
    U1REN = 1;     // Be able to receive
    SBAUD1 = (uint8_t)(256 - FREQ_SYS / 16 / UART1_BUAD);
}

/*******************************************************************************
 * Function Name  : CH554UART1RcvByte()
 * Description    : CH554UART1 Receive a byte
 * Input          : None
 * Output         : None
 * Return         : SBUF
 *******************************************************************************/
uint8_t CH554UART1RcvByte()
{
    while(U1RI == 0)
        ;    // Inquiry and receiving, the interrupt method is not used
    U1RI = 0;
    return SBUF1;
}

/*******************************************************************************
 * Function Name  : CH554UART1SendByte(uint8_t SendDat)
 * Description    : CH554UART1 Send a byte
 * Input          : uint8_t Senddat; data to be sent
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH554UART1SendByte(uint8_t SendDat)
{
    SBUF1 = SendDat;    // Inquiry sending, the interrupt method does not require the following 2 statements, but before sending TI=0
    while(U1TI == 0)
        ;
    U1TI = 1;
}

/*******************************************************************************
* Function Name  : CH554WDTModeSelect(uint8_t mode)
* Description    : CH554 Watch Dog mode selection
* Input          : uint8_t mode
                   0  timer
                   1  watchDog
* Output         : None
* Return         : None
*******************************************************************************/
void CH554WDTModeSelect(uint8_t mode)
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
* Function Name  : CH554WDTFeed(uint8_t tim)
* Description    : CH554 Set time to watch the door
* Input          : uint8_t tim Settings of the Dog resetting time
                   00H(6MHz)=2.8s
                   80H(6MHz)=1.4s
* Output         : None
* Return         : None
*******************************************************************************/
void CH554WDTFeed(uint8_t tim)
{
    WDOG_COUNT = tim;    // See the door dog counter assignment
}
