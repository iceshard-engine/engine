/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/build/platform.hxx>

#include <assert.h>
#include <cassert>
#undef assert

#if ISP_WINDOWS

#define ICE_ASSERT_CORE(expression) (void)(                                              \
        (!!(expression)) ||                                                              \
        (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
    )

#else

#define ICE_ASSERT_CORE(expression)

#endif
