#pragma once
#include <ice/data.hxx>
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice::ecs
{

    struct StorageOperationData
    {
        ice::ecs::ArchetypeComponentsInfo components_info;
        ice::Data components_data;
    };

    struct StorageOperation
    {
        ice::ecs::EntityHandle entity;
        ice::ecs::Archetype archetype;

        ice::ecs::StorageOperationData const* data = nullptr;
        ice::ecs::StorageOperation* next = nullptr;
    };

    struct StorageOperations
    {
        StorageOperations(ice::Allocator& alloc, ice::u32 prealloc) noexcept;
        ~StorageOperations() noexcept;

        ice::Allocator* _allocator;
        ice::ecs::StorageOperation* _operations;
        ice::ecs::StorageOperation* _free_operations;
    };

    StorageOperations::~StorageOperations() noexcept
    {
        ice::ecs::StorageOperation* it = _operations;
        ice::ecs::StorageOperation* allocation_head = it;

        while (it != nullptr)
        {
            it = it->next;

            if (reinterpret_cast<ice::uptr>(it->data) == 1)
            {
                _allocator->deallocate(allocation_head);
                allocation_head = it;
            }
        }

        _allocator->deallocate(allocation_head);
    }

    void storage_set_archetype(
        ice::ecs::StorageOperations& operations,
        ice::ecs::Entity entity,
        ice::ecs::Archetype archetype
    ) noexcept
    {
        if (operations._free_operations == nullptr)
        {
            ice::ecs::StorageOperation* free_operations = reinterpret_cast<ice::ecs::StorageOperation*>(operations._allocator->allocate(sizeof(StorageOperation) * 16));
            for (ice::u32 i = 2; i < 16; ++i)
            {
                free_operations[i - 1].next = free_operations + i;
            }
            free_operations[15].next = nullptr;

            // Insert the block start operation
            free_operations->next = operations._operations;
            free_operations->entity = EntityHandle::Invalid;
            free_operations->archetype = Archetype::Invalid;
            free_operations->data = reinterpret_cast<ice::ecs::StorageOperationData const*>(1);

            operations._operations = free_operations;

            // Insert other operations on the free queue
            operations._free_operations = free_operations + 1;
        }

        ice::ecs::StorageOperation* operation = operations._free_operations;
        operations._free_operations = operations._free_operations->next;

        operation->next = operations._operations;
        operations._operations = operation;
    }

} // namespace ice::ecs
