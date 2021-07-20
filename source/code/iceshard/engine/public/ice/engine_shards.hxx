#pragma once
#include <ice/shard.hxx>

namespace ice
{

    class World;

    static constexpr ice::Shard Shard_WorldActivate = "world/activate"_shard;
    static constexpr ice::Shard Shard_WorldDeactivate = "world/deactivate"_shard;

    namespace detail
    {

        template<>
        static constexpr ice::u32 Constant_ShardPayloadID<ice::World*> = ice::hash32("ice::World*");

    } // namespace detail

} // namespace ice
