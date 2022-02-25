//////////////////////////////////////////////////////////////////////

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

void user_main();
void update_10khz();
void update_tick();
void reset_usb_lines();

//////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}

//////////////////////////////////////////////////////////////////////
// C++ only bits go here

//////////////////////////////////////////////////////////////////////
// fight me

static const auto null = nullptr;

//////////////////////////////////////////////////////////////////////
// get size of an array

template <typename T, uint N> uint countof(T const (&)[N]) noexcept
{
    return N;
}

//////////////////////////////////////////////////////////////////////
// find which bit is set in a uint32.

// Used to get the bit pos for the named IO pins which stm32cube
// doesn't define for you

// could use a constexpr function but then it's tied to more
// recent compilers which might be a hassle at some point

template <uint32 x> struct bit_pos
{
    // janky static_assert polyfill to check exactly one bit is set in x
    static const char one_bit_must_be_set[(x != 0 && (x & (x - 1)) == 0) * 2 - 1];
    
    static const int pos = 1 + bit_pos<(x >> 1)>::pos;
};

template <> struct bit_pos<1>       
{
    static const int pos = 0;
};

#define BIT_POS(x) (bit_pos<(x)>::pos - 1)

//////////////////////////////////////////////////////////////////////

#endif // __cplusplus
