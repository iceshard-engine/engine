#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/cexpr/stringid.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_block_allocator.hxx>

namespace iceshard::ecs
{

    class ArchetypeIndex
    {
    public:
        virtual ~ArchetypeIndex() noexcept = default;

        virtual void add_entity(
            iceshard::Entity entity,
            core::stringid_arg_type archetype
        ) noexcept = 0;

        virtual void remove_entity(
            iceshard::Entity entity
        ) noexcept = 0;

        virtual void add_component(
            iceshard::Entity entity,
            core::stringid_arg_type component,
            uint32_t size,
            uint32_t alignment
        ) noexcept = 0;

        virtual void remove_component(
            iceshard::Entity entity,
            core::stringid_arg_type component
        ) noexcept = 0;
    };

    auto create_default_index(
        core::allocator& alloc,
        iceshard::ComponentBlockAllocator* block_alloc
    ) noexcept -> core::memory::unique_pointer<ArchetypeIndex>;

} // namespace iceshard::ecs
