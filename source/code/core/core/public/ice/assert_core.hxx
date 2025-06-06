/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/build/platform.hxx>

#include <assert.h>
#include <cassert>
#undef assert

#if ISP_WINDOWS

#define ICE_ASSERT_CORE(expression) do { if (std::is_constant_evaluated() == false) { \
        (void)(                                                                                 \
            (!!(expression)) ||                                                                 \
            (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0)    \
        ); } } while(false)

#elif ISP_WEBAPP || ISP_LINUX

#define ICE_ASSERT_CORE(expression) ((expression) \
    ? (void)0 \
    : __assert_fail(#expression, __FILE__, __LINE__, __PRETTY_FUNCTION__))

#else

#define ICE_ASSERT_CORE(expression) ((expression) \
    ? __assert_no_op \
    : __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, #expression))

#endif
