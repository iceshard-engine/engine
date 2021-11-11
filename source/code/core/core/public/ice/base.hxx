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
    using std::memset;
    using std::addressof;

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

    template<typename T>
    using clear_type_t = std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>;

    template<typename T>
    using clean_type = clear_type_t<T>;

} // namespace ice
