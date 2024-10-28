#pragma once

//////////////////////////////////////////////////////////////////////

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define U16(x) (*(uint16 *)(&(x)))

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

// where a > b but either might have wrapped
#define TICK_DIFF(a, b) ((uint16)(a - (int16)b))

#define CLEAR_CONSOLE "\033c\033[3J\033[2J"

//////////////////////////////////////////////////////////////////////

#define NOP_MACRO(...) \
    do {          \
    } while(0)

//////////////////////////////////////////////////////////////////////

const __at(ROM_CHIP_ID_LO) __code uint32 chip_id;

//////////////////////////////////////////////////////////////////////

void goto_bootloader();
void software_reset();

//////////////////////////////////////////////////////////////////////

void write_flash_data(uint8 flash_addr, uint8 len, uint8 *src_data);
void read_flash_data(uint8 flash_addr, uint8 len, uint8 *dst_data);

//////////////////////////////////////////////////////////////////////

void hard_fault();

//////////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define ASSERT(x)                                                                             \
    do {                                                                                      \
        if(!(x)) {                                                                            \
            printf("\n\nASSERT FAILED: %s\n\nAt line %d of %s:\n\n", #x, __LINE__, __FILE__); \
            while(XBUS_AUX & bUART0_TX) {                                                     \
            }                                                                                 \
            hard_fault();                                                                     \
        }                                                                                     \
    } while(0)
#else
#define ASSERT(...) NOP_MACRO(__VA_ARGS__)
#endif
