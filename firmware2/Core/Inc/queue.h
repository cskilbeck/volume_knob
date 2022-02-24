//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

template <typename T, int N> struct queue
{
    uint16 pos;
    uint16 len;

    T q[N];

    bool empty() const
    {
        return len == 0;
    }

    bool full() const
    {
        return len == N;
    }

    int space() const
    {
        return N - len;
    }

    // add new to the front of the queue

    void add(T const &item)
    {
        q[(pos + len) % N] = item;
        len += 1;
    }

    // get old from the back of the queue

    T remove()
    {
        int x = pos;
        len -= 1;
        pos = (pos + 1) % N;
        return q[x];
    }
};
