#pragma once
#include <core/build/build.hxx>
#include <core/debug/assert.hxx>

namespace core
{

    using std::addressof;

    template<typename T, uint32_t Size>
    constexpr inline auto size(T(&)[Size]) noexcept
    {
        return Size;
    }

} // namespace core
