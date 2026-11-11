/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/build/build.hxx>

#if (ICE_PROFILE || ICE_DEVELOP) && !ISP_WEBAPP

#   define IPT_ENABLED 1

    ISC_WARNING_PUSH
    ISCW_CHECK_OPERATOR_PRECEDENCE(ISCW_OP_DISABLE)
#   define TRACY_ASSERT ICE_ASSERT_CORE
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
#   define IPT_ZONE_NAME_STR( str ) ZoneName( str.begin(), str.size() )

#   define IPT_ZONE_TEXT( txt, size ) ZoneText( txt, size )
#   define IPT_ZONE_TEXT_STR( str ) IPT_ZONE_TEXT( str.begin(), str.size() )

#   define IPT_ALLOC( ptr, size ) TracyAlloc( ptr, size )
#   define IPT_DEALLOC( ptr ) TracyFree( ptr )
#   define IPT_ALLOC_POOL( ptr, size, name ) TracyAllocN( ptr, size, name.begin() )
#   define IPT_DEALLOC_POOL( ptr, name ) TracyFreeN( ptr, name.begin() )

#   define IPT_SECTION_CATEGORY( category, desc, ... ) TracySectionSetup( category, desc, ##__VA_ARGS__ )
#   define IPT_SECTION_ENTER( category, desc, ... ) TracySectionEnterCategory( category, desc, ##__VA_ARGS__ )
#   define IPT_SECTION_LEAVE( id ) TracySectionLeave( id )
#   define IPT_SECTION_SCOPED( category, desc, ... ) ice::tracy_::ScopedSection _scoped_section{ IPT_SECTION_ENTER( category, desc, ##__VA_ARGS__ ) }

#   define IUM_IPT_MESSAGE_VAR( S, txt ) TracyLogString( S, 0, 1 + TRACY_CALLSTACK, ice::tracy_::str_size(txt), ice::tracy_::str_data(txt) )
#   define IUM_IPT_MESSAGE_T( txt ) IUM_IPT_MESSAGE_VAR( ::tracy::MessageSeverity::Trace, txt )
#   define IUM_IPT_MESSAGE_D( txt ) IUM_IPT_MESSAGE_VAR( ::tracy::MessageSeverity::Debug, txt )
#   define IUM_IPT_MESSAGE_I( txt ) IUM_IPT_MESSAGE_VAR( ::tracy::MessageSeverity::Info, txt )
#   define IUM_IPT_MESSAGE_W( txt ) IUM_IPT_MESSAGE_VAR( ::tracy::MessageSeverity::Warning, txt )
#   define IUM_IPT_MESSAGE_E( txt ) IUM_IPT_MESSAGE_VAR( ::tracy::MessageSeverity::Error, txt )
#   define IUM_IPT_MESSAGE_F( txt ) IUM_IPT_MESSAGE_VAR( ::tracy::MessageSeverity::Fatal, txt )

#   define IPT_MESSAGE( S, txt ) IUM_CONCAT_MACRO( IUM_IPT_MESSAGE_, S )( txt )

#   if defined(TRACY_FIBERS)
#       define IPT_FIBERS 1
#       define IPT_FIBER_START( name ) TracyFiberEnter( name )
#       define IPT_FIBER_END TracyFiberLeave
#   else
#       define IPT_FIBERS 0
#       define IPT_FIBER_START( name )
#       define IPT_FIBER_END
#   endif


namespace ice::tracy_
{
    struct ScopedSection final
    {
        ice::u32 _internal;

        explicit ScopedSection(ice::u32 id) noexcept
            : _internal{ id }
        { }

        ~ScopedSection() noexcept
        {
            IPT_SECTION_LEAVE( _internal );
        }
    };

    template<size_t Size>
    constexpr auto str_size(char(&)[Size]) noexcept -> size_t
    {
        return Size;
    }

    constexpr auto str_size(std::string_view str) noexcept -> size_t
    {
        return str.size();
    }

    constexpr auto str_data(char const* txt) noexcept -> const char*
    {
        return txt;
    }

    constexpr auto str_data(std::string_view str) noexcept -> const char*
    {
        return str.data();
    }

} // namespace ice::_tracy

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

#   define IPT_SECTION_CATEGORY( category, desc, ... )
#   define IPT_SECTION_ENTER( category, desc, ... )
#   define IPT_SECTION_LEAVE( id )
#   define IPT_SECTION_SCOPED( category, desc, ... )

#   define IUM_IPT_MESSAGE_VAR( S, txt )
#   define IPT_MESSAGE( S, txt )

#   define IPT_FIBERS 0
#   define IPT_FIBER_START( name )
#   define IPT_FIBER_END

#endif // #if ICE_PROFILE

namespace ice::profiling
{

    static constexpr bool is_enabled = IPT_ENABLED;
    static constexpr bool has_fiber_support = IPT_FIBERS;

} // namespace ice::profiling
