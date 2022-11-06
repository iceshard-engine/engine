/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>

#if ICE_PROFILE || ICE_DEVELOP

    ISC_WARNING_PUSH
    ISCW_CHECK_OPERATOR_PRECEDENCE(ISCW_OP_DISABLE)
#   define TRACY_ENABLE
#   define TRACY_IMPORTS
#   include <tracy/Tracy.hpp>
#   undef assert
    ISC_WARNING_POP

#   define IPT_FRAME_MARK FrameMark
#   define IPT_FRAME_MARK_NAMED(...) FrameMarkNamed(__VA_ARGS__)

#   define IPT_ZONE_SCOPED ZoneScoped
#   define IPT_ZONE_SCOPED_NAMED(...) ZoneScopedN(__VA_ARGS__)

#else // #if ICE_PROFILE

#   define IPT_FRAME_MARK
#   define IPT_FRAME_MARK_NAMED(...)

#   define IPT_ZONE_SCOPED
#   define IPT_ZONE_SCOPED_NAMED(...)

#endif // #if ICE_PROFILE
