/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/build/platform.hxx>

#include <assert.h>
#include <cassert>
#undef assert

namespace ice::detail
{

    // The following function will fail to access element at index [2] only in constexpr scenarios and when the expression is false.
    // This is good enough for us to create an assertion macro for constexpr and non-constexpr cases.
    constexpr char _iceshard_constexpr_assert(bool v) noexcept
    {
        int cev = std::is_constant_evaluated();
        char constexpr_assert_failure[2]{ };
        return constexpr_assert_failure[!v + cev];
    };

} // namespace ice::detail::assert

#if ISP_WINDOWS

#define ICE_ASSERT_CORE(expression) do { if (std::is_constant_evaluated() == false) { \
        (void)(                                                                                 \
            (!!(expression)) ||                                                                 \
            (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0)    \
        ); } else { ice::detail::_iceshard_constexpr_assert(expression); } } while(false)

#elif ISP_WEBAPP || ISP_LINUX

#define ICE_ASSERT_CORE(expression) do { if (std::is_constant_evaluated() == false) { \
    ((expression) \
        ? (void)0 \
        : __assert_fail(#expression, __FILE__, __LINE__, __PRETTY_FUNCTION__)); \
    } else { ice::detail::_iceshard_constexpr_assert(expression); } } while(false)

#else

#define ICE_ASSERT_CORE(expression) do { if (std::is_constant_evaluated() == false) { \
    ((expression) \
        ? __assert_no_op \
        : __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, #expression)); \
    } else { ice::detail::_iceshard_constexpr_assert(expression); } } while(false)

#endif
