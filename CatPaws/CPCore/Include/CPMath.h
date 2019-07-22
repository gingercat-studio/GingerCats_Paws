#ifndef CATPAWS_CPCORE_CPMATH_H_
#define CATPAWS_CPCORE_CPMATH_H_

#if _MSC_VER
#include <intrin0.h>
#endif

namespace CPMath
{
 #if _MSC_VER
    inline unsigned long long ExpandToPowerOf2(unsigned long long Value)
    {
        unsigned long Index;
        _BitScanReverse64(&Index, Value - 1);
        return (1ULL << (Index + 1));
    }

#else
    inline unsigned long long ExpandToPowerOf2(unsigned long long Value)
    {
        return std::pow(2, std::ceil(std::log(Value) / std::log(2)));
    }

#endif
}

#endif
