#pragma once
#include <ice/types.hxx>
#include <ice/types_extended.hxx>
#include <ice/constants.hxx>
#include <ice/workarounds.hxx>
#include <ice/build/build.hxx>
#include <ice/assert_core.hxx>
#include <ice/config.hxx>
#include <ice/hash.hxx>

#include <ice/concept/enum_flags.hxx>
#include <ice/concept/strong_type_value.hxx>

#include <cstring>
#include <utility>
#include <bit>

namespace ice
{

    using std::min;
    using std::max;
    using std::abs;

    using std::swap;
    using std::move;
    using std::forward;
    using std::exchange;
    using std::memcpy;
    using std::memset;
    using std::addressof;

    template<typename T, ice::u32 Size>
    constexpr auto count(T const (&)[Size]) noexcept -> ice::u32
    {
        return Size;
    }

    template<typename T>
    using clear_type_t = std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>;

    template<typename T>
    using clean_type = clear_type_t<T>;

    template<typename T>
    constexpr auto to_const(T* value) noexcept -> T const*
    {
        return const_cast<T const*>(value);
    }

} // namespace ice
