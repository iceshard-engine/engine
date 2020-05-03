#pragma once
#include <inttypes.h>

namespace core
{

    using std::addressof;

    template<typename T, uint32_t Size>
    constexpr inline auto size(T(&)[Size]) noexcept
    {
        return Size;
    }

    template<typename T>
    constexpr inline auto hash(T value) noexcept -> uint64_t
    {
        return static_cast<uint64_t>(value);
    }

} // namespace core
