#pragma once
#include <ice/types.hxx>

namespace ice
{

#if ICE_PROFILE || ICE_DEVELOP

    static constexpr bool is_profiling_enabled = true;

#   define TRACY_ENABLE
#   define TRACY_IMPORTS
#   include <tracy/Tracy.hpp>
#   undef assert

#   define IPT_FRAME_MARK FrameMark
#   define IPT_FRAME_MARK_NAMED(...) FrameMarkNamed(__VA_ARGS__)

#   define IPT_ZONE_SCOPED ZoneScoped
#   define IPT_ZONE_SCOPED_NAMED(...) ZoneScopedN(__VA_ARGS__)

#else // #if ICE_PROFILE

    static constexpr bool is_profiling_enabled = false;

#   define IPT_FRAME_MARK
#   define IPT_FRAME_MARK_NAMED(...)

#   define IPT_ZONE_SCOPED
#   define IPT_ZONE_SCOPED_NAMED(...)

#endif // #if ICE_PROFILE

} // namespace ice
