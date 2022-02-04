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

#ifdef __cplusplus
}
#endif
