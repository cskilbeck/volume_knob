#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////

typedef unsigned char byte;
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

typedef unsigned int uint;

//////////////////////////////////////////////////////////////////////

#define null 0

#define countof(x) (sizeof(x) / sizeof((x)[0]))

//////////////////////////////////////////////////////////////////////

void user_main();
void read_encoder();
void button_update();
void update_tick();

//////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)

inline void divmod(uint32 val, uint32 denom, uint32 &div, uint32 &mod)
{
#if defined(__CC_ARM)
    __asm("UDIV div,val,denom");       // div = val / denom
    __asm("MLS mod,denom,div,val");    // mod = val % denom (val - (div * denom))
#else
    asm("UDIV %0,%1,%2\n" : "=r"(div) : "r"(val), "r"(denom));
    asm("MLS %0,%1,%2,%3" : "=r"(mod) : "r"(denom), "r"(div), "r"(val));
#endif
}

//////////////////////////////////////////////////////////////////////
// swap low/high 16bits within a 32bit word

inline uint32 swap_32_16(uint32 x)
{
    uint32 sh = 16;
#if defined(__CC_ARM)
    __asm("ROR x,x,sh");    // div = val / denom
#else
    asm("ROR %0,%0,%1\n" : "=r"(x) : "r"(sh));
#endif
    return x;
}

#endif


#ifdef __cplusplus
}
#endif
