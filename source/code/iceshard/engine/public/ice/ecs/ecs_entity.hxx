#pragma once
#include <ice/base.hxx>
#include <ice/shard.hxx>

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
    static constexpr bool IsEntityHandle = std::is_same_v<ice::ecs::EntityHandle, T>;

    constexpr auto entity_slot_info(
        ice::ecs::EntitySlot slot
    ) noexcept -> ice::ecs::EntitySlotInfo;

    constexpr auto entity_handle_info(
        ice::ecs::EntityHandle handle
    ) noexcept -> ice::ecs::EntityHandleInfo;


    constexpr auto entity_slot_info(
        ice::ecs::EntitySlot slot
    ) noexcept -> ice::ecs::EntitySlotInfo
    {
        union
        {
            ice::ecs::EntitySlot slot;
            ice::ecs::EntitySlotInfo info;
        } const helper{ .slot = slot };

        return helper.info;
    }

    constexpr auto entity_handle_info(
        ice::ecs::EntityHandle handle
    ) noexcept -> ice::ecs::EntityHandleInfo
    {
        union
        {
            ice::ecs::EntityHandle handle;
            ice::ecs::EntityHandleInfo info;
        } const helper{ .handle = handle };

        return helper.info;
    }

} // namespace ice::ecs

template<>
static constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::ecs::Entity> = ice::payload_id("ice::ecs::Entity");

template<>
static constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::ecs::EntityHandle> = ice::payload_id("ice::ecs::EntityHandle");
