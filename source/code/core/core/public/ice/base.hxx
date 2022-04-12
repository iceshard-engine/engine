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
    constexpr auto size(T const (&)[Size]) noexcept -> ice::u32
    {
        return Size;
    }

    template<typename T>
    using clear_type_t = std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<T>>>;

    template<typename T>
    using clean_type = clear_type_t<T>;

} // namespace ice

//! \brief This macro is required for a bug apprearing in the MSVC compile when generating optimized code with /O2
//!     It code affected is generally tied to coroutine functions / methods and seems to only occur when working with loops.
//!     A workaround for this problem is to separate the logic of such a coroutine into a separate function and mark it as 'noinline'
//!     Once the bug is fixed this macro should be removed.
//!
//! GitHub Issue: #108
#define ISATTR_NOINLINE

#if ISP_WINDOWS

#undef ISATTR_NOINLINE
#define ISATTR_NOINLINE __declspec(noinline)

#endif // #if ISP_WINDOWS
