#pragma once
#include <ice/math.hxx>
#include <ice/span.hxx>
#include <ice/shard.hxx>
#include <ice/string/string.hxx>

namespace ice::platform
{

    static constexpr ice::Shard Shard_AppQuit = "event/app/quit"_shard;
    static constexpr ice::Shard Shard_AppTerminated = "event/app/terminated"_shard;
    static constexpr ice::Shard Shard_AppSuspending = "event/app/suspending"_shard;
    static constexpr ice::Shard Shard_AppSuspended = "event/app/suspended"_shard;
    static constexpr ice::Shard Shard_AppResuming = "event/app/resuming"_shard;
    static constexpr ice::Shard Shard_AppResumed = "event/app/resumed"_shard;

    static constexpr ice::Shard Shard_WindowResized = "event/window/resized`ice::vec2u"_shard;
    static constexpr ice::Shard Shard_InputText = "event/input/text`char const*"_shard;

} // namespace ice::platform
