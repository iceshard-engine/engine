/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/log_formatters.hxx>
#include <ice/ecs/ecs_types.hxx>

namespace ice::ecs
{

    enum class Entity : ice::u32 { };

    struct EntityInfo
    {
        ice::u32 index : 24;
        ice::u32 generation : 8;
    };


    static constexpr ice::u32 Constant_EntityDataSlotArchetype_Bits = 12;
    static constexpr ice::u32 Constant_EntityDataSlotBlock_Bits = 8;
    static constexpr ice::u32 Constant_EntityDataSlotIndex_Bits = 12;

    static constexpr ice::u32 Constant_MaxArchetypeCount = 1 << Constant_EntityDataSlotArchetype_Bits;
    static constexpr ice::u32 Constant_MaxBlockCount = 1 << Constant_EntityDataSlotBlock_Bits;
    static constexpr ice::u32 Constant_MaxBlockEntityIndex = 1 << Constant_EntityDataSlotIndex_Bits;

    static_assert(
        Constant_EntityDataSlotArchetype_Bits + Constant_EntityDataSlotBlock_Bits + Constant_EntityDataSlotIndex_Bits == 32,
        "The entity slot can currently only use 32 bits of data to address a single entity. Depending on the use case this can be changed or worked around."
    );

    struct EntityDataSlot
    {
        ice::u32 archetype : Constant_EntityDataSlotArchetype_Bits;
        ice::u32 block     : Constant_EntityDataSlotBlock_Bits;
        ice::u32 index     : Constant_EntityDataSlotIndex_Bits;
    };

    static_assert(sizeof(EntityDataSlot) == sizeof(ice::u32));


    template<typename T>
    static constexpr bool IsEntity = std::is_same_v<ice::ecs::Entity, T>;

    constexpr auto entity_info(
        ice::ecs::Entity entity
    ) noexcept -> ice::ecs::EntityInfo
    {
        return std::bit_cast<ice::ecs::EntityInfo>(entity);
    }

} // namespace ice::ecs

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::ecs::Entity> = ice::shard_payloadid("ice::ecs::Entity");

template<>
struct fmt::formatter<ice::ecs::Entity>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::ecs::Entity entity, FormatContext& ctx)
    {
        constexpr ice::ecs::EntityInfo const info = ice::ecs::entity_info(entity);
        return fmt::format_to(ctx.out(), "E<{}.{}>", info.index, info.generation);
    }
};
