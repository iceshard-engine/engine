#pragma once
#include <ice/shard.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_FrameTick = "event/frame/tick"_shard;

    class World;
    static constexpr ice::Shard Shard_WorldActivate = "action/world/activate"_shard;
    static constexpr ice::Shard Shard_WorldDeactivate = "action/world/deactivate"_shard;

    static constexpr ice::Shard Shard_InputEventButton = "event/input/button"_shard;
    static constexpr ice::Shard Shard_InputEventAxis = "event/input/axis"_shard;

    namespace detail
    {

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::World*> = ice::payload_id("ice::World*");

    } // namespace detail

} // namespace ice
