#pragma once
#include <ice/data.hxx>
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice::ecs
{

    struct EntityOperationData
    {
        ice::ecs::EntityOperationData* next;

        void* operation_data;
        ice::u32 available_data_size;
    };

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

        void set_archetype(
            ice::ecs::EntityHandle entity,
            ice::ecs::Archetype archetype
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ecs::EntityOperation* _root;
        ice::ecs::EntityOperation* _operations;
        ice::ecs::EntityOperation* _free_operations;

        ice::ecs::EntityOperationData* _data_nodes;
    };

} // namespace ice::ecs
