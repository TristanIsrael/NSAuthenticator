#pragma once

#include "literals.h"

namespace ams {
    /* This comes from Atmosphère */

    namespace os {
        constexpr inline size_t MemoryPageSize = 4_KB;
        constexpr inline size_t MemoryHeapUnitSize  = 2_MB;
    }

    namespace util {
        template<typename T>
        constexpr inline T AlignUp(T value, size_t alignment) {
            using U = typename std::make_unsigned<T>::type;
            const U invmask = static_cast<U>(alignment - 1);
            return static_cast<T>((value + invmask) & ~invmask);
        }
    }
}
