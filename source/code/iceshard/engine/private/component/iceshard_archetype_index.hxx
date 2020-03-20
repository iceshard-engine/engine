#pragma once
#include <core/pod/hash.hxx>
#include <iceshard/component/component_archetype_index.hxx>
#include "component_archetype_data.hxx"

namespace iceshard::ecs
{

    namespace detail
    {

        struct ArchetypeInstance
        {
            iceshard::ecs::ArchetypeData* archetype;
            iceshard::ComponentBlock* block;
            uint32_t index;
        };

    } // namespace detail

    class IceShardArchetypeIndex : public ArchetypeIndex
    {
    public:
        IceShardArchetypeIndex(
            core::allocator& alloc,
            iceshard::ComponentBlockAllocator* block_alloc
        ) noexcept;
        ~IceShardArchetypeIndex() noexcept override;

        void add_entity(
            iceshard::Entity entity,
            core::stringid_arg_type archetype
        ) noexcept override;

        void add_entities(
            core::pod::Array<iceshard::Entity> const& entities,
            core::stringid_arg_type archetype
        ) noexcept override;

        void add_entities(
            core::pod::Array<iceshard::Entity> const& entities,
            core::stringid_arg_type archetype,
            core::pod::Array<core::data_view> const& source_blocks
        ) noexcept override;

        void remove_entity(
            iceshard::Entity entity
        ) noexcept override;

        void add_component(
            iceshard::Entity entity,
            core::stringid_arg_type component,
            uint32_t size,
            uint32_t alignment
        ) noexcept override;

        void add_component(
            iceshard::Entity entity,
            core::stringid_arg_type component,
            uint32_t size,
            uint32_t alignment,
            core::data_view initial_data
        ) noexcept override;

        void remove_component(
            iceshard::Entity entity,
            core::stringid_arg_type component
        ) noexcept override;

        auto create_archetype(
            core::pod::Array<core::stringid_type> const& components,
            core::pod::Array<uint32_t> const& sizes,
            core::pod::Array<uint32_t> const& alignments
        ) noexcept -> core::stringid_type override;

        auto get_archetype(
            core::pod::Array<core::stringid_type> const& components
        ) noexcept -> core::stringid_type override;

        void query_instances(
            core::pod::Array<core::stringid_type> const& components,
            core::pod::Array<uint32_t>& block_count,
            core::pod::Array<uint32_t>& block_offsets,
            core::pod::Array<iceshard::ComponentBlock*>& blocks
        ) noexcept override;

        bool query_instance(
            iceshard::Entity entity,
            core::pod::Array<core::stringid_type> const& components,
            core::pod::Array<uint32_t>& block_offsets,
            iceshard::ComponentBlock*& block
        ) noexcept override;

    protected:
        auto get_or_create_archetype_extended(
            iceshard::ecs::ArchetypeData const* base_archetype,
            core::stringid_arg_type component,
            uint32_t size,
            uint32_t alignment
        ) noexcept -> iceshard::ecs::ArchetypeData*;

        auto get_or_create_archetype_reduced(
            iceshard::ecs::ArchetypeData const* base_archetype,
            core::stringid_arg_type component
        ) noexcept -> iceshard::ecs::ArchetypeData*;

    private:
        core::allocator& _allocator;
        iceshard::ComponentBlockAllocator* _block_allocator;

        iceshard::ecs::ArchetypeData* _root_archetype;
        core::pod::Hash<iceshard::ecs::ArchetypeData*> _archetype_data;
        core::pod::Hash<iceshard::ecs::detail::ArchetypeInstance> _entity_archetype;
    };

} // namespace iceshard::ecs
