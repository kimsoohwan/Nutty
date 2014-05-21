#pragma once
#include "base/copy.h"

namespace nutty
{
    template <
        typename IteratorOut,
        typename T
    >
    void Copy(IteratorOut& begin, const T& t, size_t d)
    {
        nutty::base::Copy(begin, t, d);
    }

    template <
        typename IteratorOut,
        typename T
    >
    void Copy(IteratorOut& begin, const T& t)
    {
        Copy(begin, t, 1);
    }

    template <
        typename IteratorIn,
        typename IteratorOut
    >
    void Copy(IteratorOut& dst, const IteratorIn& srcBegin, const IteratorIn& srcEnd)
    {
        nutty::base::Copy(dst, srcBegin, Distance(srcBegin, srcEnd));
    }
}