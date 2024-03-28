#pragma once

//////////////////////////////////////////////////////////////////////

#define STRUCT_QUEUE(type, capacity) \
    struct                           \
    {                                \
        uint8 head;                  \
        uint8 length;                \
        type buffer[capacity];       \
    }

//////////////////////////////////////////////////////////////////////

#define QUEUE_CAPACITY(q) (sizeof(q.buffer) / sizeof(q.buffer[0]))
#define QUEUE_SPACE(q) (QUEUE_CAPACITY(q) - q.length)
#define QUEUE_LENGTH(q) (q.length)
#define QUEUE_IS_EMPTY(q) (q.length == 0)
#define QUEUE_IS_FULL(q) (QUEUE_CAPACITY(q) == q.length)

//////////////////////////////////////////////////////////////////////

#define QUEUE_INIT(q)                                                  \
    STATIC_ASSERT((QUEUE_CAPACITY(q) & (QUEUE_CAPACITY(q) - 1)) == 0); \
    do {                                                               \
        q.head = 0;                                                    \
        q.length = 0;                                                  \
    } while(0)

//////////////////////////////////////////////////////////////////////

#define QUEUE_PUSH(q, v)                                           \
    STATIC_ASSERT(sizeof(v) == sizeof(q.buffer[0]));               \
    do {                                                           \
        q.buffer[(q.head + q.length++) % QUEUE_CAPACITY(q)] = (v); \
    } while(0)

//////////////////////////////////////////////////////////////////////

#define QUEUE_POP(q, v)                              \
    STATIC_ASSERT(sizeof(v) == sizeof(q.buffer[0])); \
    do {                                             \
        (v) = q.buffer[q.head];                      \
        q.length -= 1;                               \
        q.head = ++q.head % QUEUE_CAPACITY(q);       \
    } while(0)

/* USAGE:

typedef STRUCT_QUEUE(int, 16) int_queue_t;

int_queue_t my_queue;

QUEUE_INIT(my_queue);

QUEUE_PUSH(my_queue, 10);
QUEUE_PUSH(my_queue, 11);
QUEUE_PUSH(my_queue, 12);

ASSERT(QUEUE_LENGTH(my_queue) == 3);

int a, b, c;
QUEUE_POP(my_queue, a);
QUEUE_POP(my_queue, b);
QUEUE_POP(my_queue, c);

*/
