#pragma once
#include <ice/build/build.hxx>
#include <ice/math.hxx>

#include <type_traits>
#include <cstring>
#include <utility>

namespace ice
{

    using std::min;
    using std::max;

    using std::swap;
    using std::move;
    using std::forward;
    using std::exchange;
    using std::memcpy;

    template<typename T>
    constexpr auto size(T const& cont) noexcept -> decltype(cont._size)
    {
        return cont._size;
    }

    template<typename T, ice::u32 Size>
    constexpr auto size(T const(&)[Size]) noexcept -> ice::u32
    {
        return Size;
    }

} // namespace ice
