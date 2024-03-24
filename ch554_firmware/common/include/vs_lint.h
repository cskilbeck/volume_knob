// NOT #pragma once

#if defined(VSCODE)

#define STATIC_ASSERT(...)
#define SIZEOF_LSB(x) 0xff
#define SIZEOF_MSB(x) 0xff

#define __data
#define __near
#define __idata
#define __xdata
#define __far
#define __pdata
#define __code
#define __bit bool
#define __sfr volatile unsigned char
#define __sbit volatile bool
#define __critical
#define __at(x) /* use "__at (0xab)" instead of "__at 0xab" */
#define __using(x)
#define __interrupt(x)
#define __naked

#else

#define STATIC_ASSERT _Static_assert
#define SIZEOF_LSB(x) (sizeof(x) & 0xff)
#define SIZEOF_MSB(x) (sizeof(x) >> 8)

#endif
