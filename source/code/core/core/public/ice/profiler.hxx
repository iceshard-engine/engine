/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>

#if ICE_PROFILE && !ISP_WEBAPP

#   define IPT_ENABLED 1

    ISC_WARNING_PUSH
    ISCW_CHECK_OPERATOR_PRECEDENCE(ISCW_OP_DISABLE)
#   define TRACY_ENABLE
#   define TRACY_IMPORTS
#   include <tracy/Tracy.hpp>
#   include <tracy/TracyC.h>
#   undef assert
    ISC_WARNING_POP

#   define IPT_FRAME_MARK FrameMark
#   define IPT_FRAME_MARK_NAMED( ... ) FrameMarkNamed( __VA_ARGS__ )

#   define IPT_ZONE_SCOPED ZoneScoped
#   define IPT_ZONE_SCOPED_NAMED( ... ) ZoneScopedN( __VA_ARGS__ )
#   define IPT_ZONE_NAME_STR( str ) ZoneName( ice::string::begin(str), ice::string::size(str) )

#   define IPT_ZONE_TEXT( txt, size ) ZoneText( txt, size )
#   define IPT_ZONE_TEXT_STR( str ) IPT_ZONE_TEXT( ice::string::begin(str), ice::string::size(str) )

#   define IPT_ALLOC( ptr, size ) TracyAlloc( ptr, size )
#   define IPT_DEALLOC( ptr ) TracyFree( ptr )
#   define IPT_ALLOC_POOL( ptr, size, name ) TracyAllocN( ptr, size, ice::string::begin(name) )
#   define IPT_DEALLOC_POOL( ptr, name ) TracyFreeN( ptr, ice::string::begin(name) )

#   define IPT_MESSAGE( txt ) TracyMessage( txt, ice::count(txt) )

#else // #if ICE_PROFILE

#   define IPT_ENABLED 0

#   define IPT_FRAME_MARK
#   define IPT_FRAME_MARK_NAMED(...)

#   define IPT_ZONE_SCOPED
#   define IPT_ZONE_SCOPED_NAMED(...)
#   define IPT_ZONE_NAME_STR( str )

#   define IPT_ZONE_TEXT( txt, size )
#   define IPT_ZONE_TEXT_STR( str )

#   define IPT_ALLOC( ptr, size )
#   define IPT_DEALLOC( ptr )
#   define IPT_ALLOC_POOL( ptr, size, name )
#   define IPT_DEALLOC_POOL( ptr, name )

#   define IPT_MESSAGE( txt )

#endif // #if ICE_PROFILE
