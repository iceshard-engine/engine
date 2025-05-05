/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/log_formatters.hxx>
#include <ice/ecs/ecs_types.hxx>

namespace ice::ecs
{

    enum class Entity : ice::u32 { };

    enum class EntitySlot : ice::u32
    {
        Invalid = 0x0
    };

    enum class EntityHandle : ice::u64
    {
        Invalid = 0x0
    };

    struct EntityInfo
    {
        ice::u32 index : 24;
        ice::u32 generation : 8;
    };


    static constexpr ice::u32 Constant_EntitySlotArchetype_Bits = 12;
    static constexpr ice::u32 Constant_EntitySlotBlock_Bits = 8;
    static constexpr ice::u32 Constant_EntitySlotIndex_Bits = 12;

    static constexpr ice::u32 Constant_MaxArchetypeCount = 1 << Constant_EntitySlotArchetype_Bits;
    static constexpr ice::u32 Constant_MaxBlockCount = 1 << Constant_EntitySlotBlock_Bits;
    static constexpr ice::u32 Constant_MaxBlockEntityIndex = 1 << Constant_EntitySlotIndex_Bits;

    static_assert(
        Constant_EntitySlotArchetype_Bits + Constant_EntitySlotBlock_Bits + Constant_EntitySlotIndex_Bits == 32,
        "The entity slot can currently only use 32 bits of data to address a single entity. Depending on the use case this can be changed or worked around."
    );


    struct EntitySlotInfo
    {
        ice::u32 archetype : Constant_EntitySlotArchetype_Bits;
        ice::u32 block     : Constant_EntitySlotBlock_Bits;
        ice::u32 index     : Constant_EntitySlotIndex_Bits;
    };

    struct EntityHandleInfo
    {
        ice::ecs::Entity entity;
        ice::ecs::EntitySlot slot;
    };

    static_assert(sizeof(EntitySlotInfo) == sizeof(EntitySlot));
    static_assert(sizeof(EntityHandleInfo) == sizeof(EntityHandle));


    template<typename T>
    static constexpr bool IsEntity = std::is_same_v<ice::ecs::Entity, T>;

    template<typename T>
    static constexpr bool IsEntityHandle = std::is_same_v<ice::ecs::EntityHandle, T>;


    constexpr auto entity_info(
        ice::ecs::Entity entity
    ) noexcept -> ice::ecs::EntityInfo;

    constexpr auto entity_slot_info(
        ice::ecs::EntitySlot slot
    ) noexcept -> ice::ecs::EntitySlotInfo;

    constexpr auto entity_handle_info(
        ice::ecs::EntityHandle handle
    ) noexcept -> ice::ecs::EntityHandleInfo;


    constexpr auto entity_info(
        ice::ecs::Entity entity
    ) noexcept -> ice::ecs::EntityInfo
    {
        return std::bit_cast<ice::ecs::EntityInfo>(entity);
    }

    constexpr auto entity_slot_info(
        ice::ecs::EntitySlot slot
    ) noexcept -> ice::ecs::EntitySlotInfo
    {
        return std::bit_cast<ice::ecs::EntitySlotInfo>(slot);
    }

    constexpr auto entity_handle_info(
        ice::ecs::EntityHandle handle
    ) noexcept -> ice::ecs::EntityHandleInfo
    {
        return std::bit_cast<ice::ecs::EntityHandleInfo>(handle);
    }

} // namespace ice::ecs

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::ecs::Entity> = ice::shard_payloadid("ice::ecs::Entity");

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::ecs::EntityHandle> = ice::shard_payloadid("ice::ecs::EntityHandle");

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
        return fmt::format_to(ctx.out(), "<{}>", static_cast<std::underlying_type_t<ice::ecs::Entity>>(entity));
    }
};

template<>
struct fmt::formatter<ice::ecs::EntityHandle>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    constexpr auto format(ice::ecs::EntityHandle handle, FormatContext& ctx)
    {
        ice::ecs::EntityHandleInfo const handle_info = ice::ecs::entity_handle_info(handle);
        ice::ecs::EntitySlotInfo const slot_info = ice::ecs::entity_slot_info(handle_info.slot);

        return fmt::format_to(
            ctx.out(),
            "<{}/{}>@<{}.{}>",
            slot_info.archetype,
            static_cast<std::underlying_type_t<ice::ecs::Entity>>(handle_info.entity),
            slot_info.block,
            slot_info.index
        );
    }
};
