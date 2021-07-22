#pragma once
#include <ice/shard.hxx>

namespace ice
{

    class World;
    static constexpr ice::Shard Shard_WorldActivate = "action/world/activate"_shard;
    static constexpr ice::Shard Shard_WorldDeactivate = "action/world/deactivate"_shard;

    enum class Entity : ice::u64;
    static constexpr ice::Shard Shard_EntityDestroy = "action/entity/destroy"_shard;

    static constexpr ice::Shard Shard_EntityCreated = "event/entity/created"_shard;
    static constexpr ice::Shard Shard_EntityDestroyed = "event/entity/destroyed"_shard;

    namespace detail
    {

        template<>
        static constexpr ice::u32 Constant_ShardPayloadID<ice::World*> = ice::hash32("ice::World*");

        template<>
        static constexpr ice::u32 Constant_ShardPayloadID<ice::Entity> = ice::hash32("ice::Entity");

    } // namespace detail

} // namespace ice
