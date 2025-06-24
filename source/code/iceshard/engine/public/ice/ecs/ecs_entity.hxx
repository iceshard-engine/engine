/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_concepts.hxx>
#include <ice/log_formatters.hxx>

namespace ice::ecs
{

    //! \brief Primary identifier for entities in IceShard. Used to update data using operations or access components with queries.
    //!
    //! \details An Entity handle is opaque and does not provide any functionality out of the box. It's mainly used as an argument in other ECS related
    //!   systems. For examples, you can check if entities are alive with `EntityIndex::is_alive` or query a specific components with `Query` objects.
    enum class Entity : ice::u32 { Invalid = 0 };

    //! \brief Provides easy access to the specific parts making up the `Entity` handle value.
    struct EntityInfo
    {
        //! \brief Describes the entity's direct ID which can be also used as an index into tables that always track all entities.
        //!
        //! \remark An entity's index will only be reused after a certain amount of entities are destroyed. Currently the cutoff is at
        //!   1024 entities. This means that the same handle will be generated at earliest when `1024 * 256 (generation max value)` entities
        //!   are cycled through.
        ice::u32 index : 24;

        //! \brief Describes the entity's generation (aka. version) that is used to determine in an entity was destroyed or not.
        //!
        //! \note Once an entity is destroyed, the `EntityIndex` will increase the internal generation value for that entity.
        //!   This ensures that checks with `EntityIndex::is_alive` are quick because they compare the given `Entity` generation with the
        //!   internal generation. If the values missmatch the entity is considered destroyed.
        ice::u32 generation : 8;
    };

    //! \brief Extracts details from an entity handle.
    //! \param entity Handle to the data should be extracted from.
    //! \return information extracted from the Entity handle.
    constexpr auto entity_info(ice::ecs::Entity entity) noexcept -> ice::ecs::EntityInfo;


    //! \brief Number of bits reserved to identify the entity's `Archetype` in an `EntityStorage`. Allows for 4095 archerypes
    static constexpr ice::u32 Constant_EntityDataSlotArchetype_Bits = 12;

    //! \brief Number of bits reserved to identify the internal data block where the entity data is stored. Allows for an `Archetype` to
    //!   have up to 256 data blocks.
    //!
    //! \remark Because the amount of entities a default allocated block can store differs depending the component count and sizes
    //!   it might be necessary to provide custom allocators for extremely large `Archetype`s to not run out of `DataBlocks` if such
    //!   blocks end up to only hold 50 entities or even less, \see `EntityStorage` for more details.
    static constexpr ice::u32 Constant_EntityDataSlotBlock_Bits = 8;

    //! \brief Number of bits reserved to index into the tables stored in a single `DataBlock`. Allows to store 4096 entities in each data block.
    //!
    //! \remark Because each `Archerype` may have 256 blocks and each can hold 4096 entities, the total entity count each archetypes can hold ends up
    //!   to be 1,048,576. If this number is not enough you may consider to relocate some of the bits for your convenience.
    //!
    //! \note It might be possible to change the base type of `EntityDataSlot` to 64 bits, however this option was not tested.
    static constexpr ice::u32 Constant_EntityDataSlotIndex_Bits = 12;

    //! \brief Maximum number of archetypes allowed in the system.
    //! \remark Archetype at index '0' is considered the 'NullArchetype'
    static constexpr ice::u32 Constant_MaxArchetypeCount = (1 << Constant_EntityDataSlotArchetype_Bits) - 1;

    //! \brief Maximum number of blocks tracked by a single archetype.
    static constexpr ice::u32 Constant_MaxBlockCount = 1 << Constant_EntityDataSlotBlock_Bits;

    //! \brief Maximum number of entities indexable in a single data block.
    static constexpr ice::u32 Constant_MaxBlockEntityIndex = 1 << Constant_EntityDataSlotIndex_Bits;

    static_assert(
        Constant_EntityDataSlotArchetype_Bits + Constant_EntityDataSlotBlock_Bits + Constant_EntityDataSlotIndex_Bits == 32,
        "The entity slot can currently only use 32 bits of data to address a single entity. Depending on the use case this can be changed or worked around."
    );

    //! \brief Describes the location where the entity data is stored for a specific `EntityStorage` implementation.
    //!   There shouldn't be any case where users of the API will need to directly access this information.
    //!
    //! \remarks (2025.05) The current storage implementation does not grant access to all components of a specific entity with just the
    //!   information available in this structure. You will also need the `ArchetypeInstanceInfo` description to access the right locations.
    struct EntityDataSlot
    {
        ice::u32 archetype : Constant_EntityDataSlotArchetype_Bits;
        ice::u32 block     : Constant_EntityDataSlotBlock_Bits;
        ice::u32 index     : Constant_EntityDataSlotIndex_Bits;
    };

    static_assert(sizeof(EntityDataSlot) == sizeof(ice::u32));

    constexpr auto entity_info(ice::ecs::Entity entity) noexcept -> ice::ecs::EntityInfo
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
        ice::ecs::EntityInfo const info = ice::ecs::entity_info(entity);
        return fmt::format_to(ctx.out(), "E<{}.{}>", info.index, info.generation);
    }
};
