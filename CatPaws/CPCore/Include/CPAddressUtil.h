#ifndef CATPAWS_CPCORE_CPADDRESSUTIL_H_
#define CATPAWS_CPCORE_CPADDRESSUTIL_H_

namespace AddressUtil {
    inline void* AlignFoward(void* address, uint8_t alignment)
    {
        return (void*)
            ((reinterpret_cast<uintptr_t>(address) +
                static_cast<uintptr_t>(alignment - 1)) &
                static_cast<uintptr_t>(~(alignment - 1)));
    }

    inline uintptr_t AlignFowardAdjustment(void* address, uint8_t alignment)
    {
        auto adjustment = alignment -
            (reinterpret_cast<uintptr_t>(address) &
                static_cast<uintptr_t>(alignment - 1));

        if (adjustment == alignment) return 0;
        return adjustment;
    }

    inline uintptr_t AlignFowardAdjustmentWithHeader(
        void* address, uint8_t alignment, uint8_t headersize)
    {
        auto adjustment = AlignFowardAdjustment(address, alignment);
        uintptr_t neededspace = headersize;

        if (adjustment < neededspace)
        {
            neededspace -= adjustment;
            adjustment += static_cast<uintptr_t>(alignment) *
                static_cast<uintptr_t>((neededspace / alignment));
            if ((neededspace % alignment) > 0)
            {
                adjustment += alignment;
            }
        }

        return adjustment;
    }

    inline void* MovePtr(void* ptr, std::size_t amount)
    {
        return (void*)(reinterpret_cast<uintptr_t>(ptr) + amount);
    }

    inline const void* MovePtr(const void* ptr, std::size_t amount)
    {
        return (const void*)(reinterpret_cast<uintptr_t>(ptr) + amount);
    }
}

#endif /*CATPAWS_CPCORE_CPADDRESSUTIL_H_*/
