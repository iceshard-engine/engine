#pragma once
#include <ice/shard.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_FrameTick = "event/frame/tick"_shard;

    class World;
    static constexpr ice::Shard Shard_WorldActivate = "action/world/activate"_shard;
    static constexpr ice::Shard Shard_WorldDeactivate = "action/world/deactivate"_shard;

    enum class Entity : ice::u64;
    static constexpr ice::Shard Shard_EntityDestroy = "action/entity/destroy"_shard;

    static constexpr ice::Shard Shard_EntityCreated = "event/entity/created"_shard;
    static constexpr ice::Shard Shard_EntityDestroyed = "event/entity/destroyed"_shard;

    static constexpr ice::Shard Shard_InputEventButton = "event/input/button"_shard;
    static constexpr ice::Shard Shard_InputEventAxis = "event/input/axis"_shard;

    namespace detail
    {

        template<>
        static constexpr ice::PayloadID Constant_ShardPayloadID<ice::World*> = ice::payload_id("ice::World*");

        template<>
        static constexpr ice::PayloadID Constant_ShardPayloadID<ice::Entity> = ice::payload_id("ice::Entity");

    } // namespace detail

} // namespace ice
