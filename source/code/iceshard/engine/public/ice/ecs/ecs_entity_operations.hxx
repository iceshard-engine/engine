#pragma once
#include <ice/data.hxx>
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice::ecs
{

    static constexpr ice::Shard Shard_EntityCreated = "event/ecs-v2/entity-created"_shard;
    static constexpr ice::Shard Shard_EntityDestroyed = "event/ecs-v2/entity-destroyed"_shard;
    static constexpr ice::Shard Shard_EntityHandleChanged = "event/ecs-v2/entity-handle-changed"_shard;
    static constexpr ice::Shard Shard_EntityArchetypeChanged = "event/ecs-v2/entity-archetype-changed"_shard;

    struct EntityOperation
    {
        ice::ecs::EntityOperation* next;

        ice::u32 entity_count : 31;
        ice::u32 notify_entity_changes : 1;
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

        void clear() noexcept;
        void grow(ice::u32 count) noexcept;

        auto new_storage_operation() noexcept -> ice::ecs::EntityOperation*;

        auto new_storage_operation(
            ice::u32 required_data_size,
            void*& out_operation_data_ptr
        ) noexcept -> ice::ecs::EntityOperation*;

        struct ComponentInfo
        {
            ice::Span<ice::StringID const> names;
            ice::Span<ice::u32 const> sizes;
            ice::Span<ice::u32 const> offsets;
        };

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
        ice::ecs::Entity entity,
        ice::ecs::Archetype archetype
    ) noexcept;

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        bool notify_changes = false
    ) noexcept;

    void queue_remove_entity(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::EntityHandle entity_handle
    ) noexcept;

} // namespace ice::ecs
