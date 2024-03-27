#pragma once

//////////////////////////////////////////////////////////////////////

#define STRUCT_QUEUE(type, capacity) \
    struct                           \
    {                                \
        uint8 head;                  \
        uint8 size;                  \
        type buffer[capacity];       \
    }

//////////////////////////////////////////////////////////////////////

#define QUEUE_SIZE(q) (sizeof(q.buffer) / sizeof(q.buffer[0]))

#define QUEUE_SPACE(q) (QUEUE_SIZE(q) - q.size)

#define QUEUE_IS_EMPTY(q) (q.size == 0)

#define QUEUE_IS_FULL(q) (QUEUE_SIZE(q) == q.size)

//////////////////////////////////////////////////////////////////////

#define QUEUE_INIT(q)                                          \
    STATIC_ASSERT((QUEUE_SIZE(q) & (QUEUE_SIZE(q) - 1)) == 0); \
    do {                                                       \
        q.head = 0;                                            \
        q.size = 0;                                            \
    } while(0)

//////////////////////////////////////////////////////////////////////

#define QUEUE_PUSH(q, v) q.buffer[(q.head + q.size++) & (QUEUE_SIZE(q) - 1)] = (v)

//////////////////////////////////////////////////////////////////////

#define QUEUE_POP(q, v)                          \
    do {                                         \
        (v) = q.buffer[q.head];                  \
        q.size -= 1;                             \
        q.head = ++q.head & (QUEUE_SIZE(q) - 1); \
    } while(0)
