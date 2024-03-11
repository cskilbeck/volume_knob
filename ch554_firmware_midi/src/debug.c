//////////////////////////////////////////////////////////////////////

#include "main.h"

#define _BAUD_X1(clk, baud) (10 * clk / baud / 16)
#define _BAUD_X2(clk, baud) (_BAUD_X1(clk, baud) % 10)
#define _BAUD_X3(clk, baud) (_BAUD_X2(clk, baud) / 5)
#define _BAUD_X4(clk, baud) ((_BAUD_X1(clk, baud) / 10) + _BAUD_X3(clk, baud))

#define BAUD_SET(clk, baud) ((uint8)(0x100 - _BAUD_X4(clk, baud)))

//////////////////////////////////////////////////////////////////////

void clk_init()
{
#if defined(EXTERNAL_OSCILLATOR)
#define OSC_ON bOSC_EN_XT
#else
#define OSC_ON bOSC_EN_INT
#endif

#if !defined(FREQ_SYS)
#define FREQ_SYS 24000000
#endif

#if FREQ_SYS == 32000000
#define CLK 0x07
#elif FREQ_SYS == 24000000
#define CLK 0x06
#elif FREQ_SYS == 16000000
#define CLK 0x05
#elif FREQ_SYS == 12000000
#define CLK 0x04
#elif FREQ_SYS == 6000000
#define CLK 0x03
#elif FREQ_SYS == 3000000
#define CLK 0x02
#elif FREQ_SYS == 750000
#define CLK 0x01
#elif FREQ_SYS == 187500
#define CLK 0x00
#else
#warning FREQ_SYS invalid or not set
#endif

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG = (CLOCK_CFG & ~(MASK_SYS_CK_SEL | bOSC_EN_INT | bOSC_EN_XT)) | (OSC_ON | CLK);
    SAFE_MOD = 0x00;
}

//////////////////////////////////////////////////////////////////////

void delay_uS(uint16_t n)
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

//////////////////////////////////////////////////////////////////////

void delay_mS(uint16_t n)
{
    while(n) {
        delay_uS(1000);
        --n;
    }
}

//////////////////////////////////////////////////////////////////////

void uart0_alternate_pins()
{
    PIN_FUNC |= bUART0_PIN_X;    // Map in serial port to P1.2 and P1.3
}

//////////////////////////////////////////////////////////////////////

void uart0_init()
{
    SM0 = 0;                                                      // 8 bits
    SM1 = 1;                                                      // mode 1
    SM2 = 0;                                                      // SM2 not relevant in mode 1
    RCLK = 0;                                                     // receive use T1
    TCLK = 0;                                                     // send use T1
    PCON |= SMOD;                                                 // fast baud mode
    TMOD = TMOD & ~(bT1_GATE | bT1_CT | MASK_T1_MOD) | bT1_M1;    // Timer 1 8-bit automatic load
    T2MOD |= bTMR_CLK | bT1_CLK;                                  // Timer 1 clock select
    TH1 = BAUD_SET(FREQ_SYS, UART0_BAUD);                         // baud rate
    TI = 1;                                                       // Set transmit done flag so we don't wait indefinitely before sending 1st byte
    RI = 0;                                                       // clear receive flag
    TR1 = 1;                                                      // start T1
    REN = 1;                                                      // receive enable
}

//////////////////////////////////////////////////////////////////////

uint8_t uart0_get_gyte()
{
    while(RI == 0) {
    }
    RI = 0;
    return SBUF;
}

//////////////////////////////////////////////////////////////////////

void uart0_send_byte(uint8_t SendDat)
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

//////////////////////////////////////////////////////////////////////

void uart1_init()
{
    U1SM0 = 0;     // UART1 Select 8 -bit data bit
    U1SMOD = 1;    // Fast mode
    U1REN = 1;     // Be able to receive
    SBAUD1 = BAUD_SET(FREQ_SYS, UART1_BUAD);
}

//////////////////////////////////////////////////////////////////////

uint8_t uart1_get_gyte()
{
    while(U1RI == 0) {
    }
    U1RI = 0;
    return SBUF1;
}

//////////////////////////////////////////////////////////////////////

void uart1_send_byte(uint8_t SendDat)
{
    SBUF1 = SendDat;
    while(U1TI == 0) {
    }
    U1TI = 1;
}

//////////////////////////////////////////////////////////////////////

void watchdog_mode_select(uint8_t mode)
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xaa;
    if(mode) {
        GLOBAL_CFG |= bWDOG_EN;
    } else
        GLOBAL_CFG &= ~bWDOG_EN;
    SAFE_MOD = 0x00;
    WDOG_COUNT = 0;
}

//////////////////////////////////////////////////////////////////////

void watchdog_feed(uint8_t tim)
{
    WDOG_COUNT = tim;
}

//////////////////////////////////////////////////////////////////////

#if DEVICE == DEVICE_DEVKIT
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

void print_uint8(char *msg, uint8 v)
{
    putstr(msg);
    putstr(": U8 = 0x");
    puthex(v);
    putstr("\n");
}

//////////////////////////////////////////////////////////////////////

void print_uint16(char *msg, uint16 v)
{
    putstr(msg);
    putstr(": U16 = 0x");
    puthex(v >> 8);
    puthex(v & 0xff);
    putstr("\n");
}

//////////////////////////////////////////////////////////////////////

void print_uint32(char *msg, uint32 v)
{
    putstr(msg);
    putstr(": U32 = 0x");
    puthex((v >> 24) & 0xff);
    puthex((v >> 16) & 0xff);
    puthex((v >> 8) & 0xff);
    puthex(v & 0xff);
    putstr("\n");
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
#endif
