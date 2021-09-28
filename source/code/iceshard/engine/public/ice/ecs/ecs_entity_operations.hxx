#pragma once
#include <ice/data.hxx>
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice::ecs
{

    // Set Archetype: {EntityHandle[], DstArchetype, ComponentData[]} // set
    // Rep Archetype: {EntityHandle[], DstArchetype, <implicit: SrcArchetype>, ComponentData[]} // change
    // Set Component: {EntityHandle[], None, ComponentData[]} // update data
    // Set Component: {EntityHandle[], None} // remove

    struct EntityOperation
    {
        ice::ecs::EntityOperation* next;

        ice::u32 entity_count;
        ice::u32 component_data_size;

        ice::ecs::Archetype archetype;
        ice::ecs::EntityHandle* entities;
        void* component_data;
    };

    class EntityOperations
    {
    public:
        EntityOperations(ice::Allocator& alloc, ice::u32 prealloc = 16) noexcept;
        ~EntityOperations() noexcept;

        void grow(ice::u32 count) noexcept;

        auto new_storage_operation() noexcept -> ice::ecs::EntityOperation*;

        auto new_storage_operation(
            ice::u32 required_data_size,
            void*& out_operation_data_ptr
        ) noexcept -> ice::ecs::EntityOperation*;

        void set_archetype(
            ice::ecs::EntityHandle entity,
            ice::ecs::Archetype archetype
        ) noexcept;


        struct EntityOperationData;
        struct OperationIterator
        {
            auto operator*() const noexcept -> EntityOperation const&;
            bool operator==(OperationIterator const& other) const noexcept;
            bool operator!=(OperationIterator const& other) const noexcept;
            auto operator++() noexcept -> OperationIterator&;

            ice::ecs::EntityOperation const* _entry;
        };

        auto begin() const noexcept -> OperationIterator;
        auto end() const noexcept -> OperationIterator;

    private:
        ice::Allocator& _allocator;
        ice::ecs::EntityOperation* _root;
        ice::ecs::EntityOperation* _operations;
        ice::ecs::EntityOperation* _free_operations;

        EntityOperationData* _data_nodes;
    };

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::EntityHandle entity,
        ice::ecs::Archetype archetype
    ) noexcept;

} // namespace ice::ecs
