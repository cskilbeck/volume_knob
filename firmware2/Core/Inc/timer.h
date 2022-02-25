#pragma once

#include "user.h"

//////////////////////////////////////////////////////////////////////

extern uint32 ticks10khz;

//////////////////////////////////////////////////////////////////////

struct timer
{
    // when the timer was last reset
    uint32 base;

    // how many 1/10ths of a millisecond before it expires
    uint32 alarm;

    timer()
    {
    }

    explicit timer(uint32 expires)
    {
        reset(expires);
    }

    // reset to now with expiry time
    void reset(uint32 expires)
    {
        base = ticks10khz;
        alarm = expires;
    }

    // how many 1/10th of a millisecond since it was reset
    uint32 elapsed() const
    {
        return ticks10khz - base;
    }

    // has the expiry time been reached?
    bool expired() const
    {
        return elapsed() >= alarm;
    }
};

//////////////////////////////////////////////////////////////////////
