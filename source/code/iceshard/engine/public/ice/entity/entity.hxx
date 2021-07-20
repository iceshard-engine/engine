#pragma once
#include <ice/base.hxx>
#include <ice/shard.hxx>

namespace ice
{

    enum class Entity : ice::u64 { };

    struct alignas(8) EntityInfo
    {
        ice::u32 index;
        ice::u16 generation;
        ice::u16 reserved;
    };

    static_assert(
        sizeof(Entity) == sizeof(EntityInfo),
        "EntityInfo and Entity types are required to have same size in memory!"
    );

    static_assert(
        alignof(Entity) == alignof(EntityInfo),
        "EntityInfo and Entity types are required to have same alignment in memory!"
    );

} // namespace ice

template<>
static constexpr ice::u32 ice::detail::Constant_ShardPayloadID<ice::Entity> = ice::hash32("ice::Entity");
