/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    struct EntityOperations::EntityOperationData
    {
        ice::usize allocated_size;
        ice::usize available_size;
        void* operation_data;

        EntityOperationData* next;
    };

    auto EntityOperations::OperationIterator::operator*() const noexcept -> EntityOperation const&
    {
        return *_entry;
    }

    bool EntityOperations::OperationIterator::operator==(OperationIterator const& other) const noexcept
    {
        return _entry == other._entry;
    }

    bool EntityOperations::OperationIterator::operator!=(OperationIterator const& other) const noexcept
    {
        return !(*this == other);
    }

    auto EntityOperations::OperationIterator::operator++() noexcept -> OperationIterator&
    {
        _entry = _entry->next;

        // We need to skip over operations that are acting as block heads
        while (_entry != nullptr && _entry->entities == reinterpret_cast<EntityHandle const*>(1))
        {
            _entry = _entry->next;
        }
        return *this;
    }

    namespace detail
    {

        auto make_empty_entity_handle(
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::EntityHandle
        {
            ice::ecs::EntityHandleInfo const info{
                    .entity = entity,
                    .slot = ice::ecs::EntitySlot::Invalid
            };
            return std::bit_cast<ice::ecs::EntityHandle>(info);
        }

        auto allocate_operation_nodes(
            ice::Allocator& alloc,
            ice::u32 count
        ) noexcept -> ice::ecs::EntityOperation*
        {
            ice::AllocResult const node_data = alloc.allocate(ice::meminfo_of<ice::ecs::EntityOperation> * (count + 1));

            EntityOperation* operations = reinterpret_cast<EntityOperation*>(node_data.memory);
            for (ice::u32 idx = 1; idx <= count; ++idx)
            {
                operations[idx - 1].next = operations + idx;
            }
            operations[count].next = nullptr;

            // Make the first operation empty and mark it as an allocation guard node.
            operations->entity_count = 0;
            operations->component_data_size = 0;
            operations->archetype = Archetype::Invalid;
            operations->entities = reinterpret_cast<ice::ecs::EntityHandle*>(1); // Allocation guard node
            operations->component_data = reinterpret_cast<void*>(node_data.size.value);
            return operations;
        }

        auto allocate_data_node(
            ice::Allocator& alloc,
            ice::usize data_block_size,
            ice::ecs::EntityOperations::EntityOperationData* previous_node
        ) noexcept -> ice::ecs::EntityOperations::EntityOperationData*
        {
            ice::Memory const memory = alloc.allocate(
                AllocRequest{
                    ice::size_of<ice::ecs::EntityOperations::EntityOperationData> + data_block_size,
                    ice::align_of<ice::ecs::EntityOperations::EntityOperationData>
                }
            );

            EntityOperations::EntityOperationData* data_node = reinterpret_cast<EntityOperations::EntityOperationData*>(memory.location);
            data_node->allocated_size = memory.size;
            data_node->available_size = data_block_size;
            data_node->operation_data = data_node + 1;
            data_node->next = previous_node;
            return data_node;
        }

    } // namespace detail

    EntityOperations::EntityOperations(
        ice::Allocator& alloc,
        ice::u32 initial_count
    ) noexcept
        : _allocator{ alloc }
        , _operations{ nullptr }
        , _free_operations{ nullptr }
        , _data_nodes{ detail::allocate_data_node(_allocator, 16_KiB, nullptr) }
    {
        ice::ecs::EntityOperation* const new_operations = detail::allocate_operation_nodes(
            _allocator, initial_count
        );

        _root = new_operations;
        _operations = new_operations;
        _free_operations = _operations->next;
        _operations->next = nullptr;
    }

    EntityOperations::~EntityOperations() noexcept
    {
        ice::ecs::EntityOperation* it = _root;
        ice::ecs::EntityOperation* allocation_node = it;

        while (it->next != nullptr)
        {
            it = it->next;

            if (reinterpret_cast<ice::uptr>(it->entities) == 1)
            {
                // Delete the previous known memory of nodes.
                _allocator.deallocate(
                    ice::Memory{
                        .location = allocation_node,
                        .size = { reinterpret_cast<ice::usize::base_type>(allocation_node->component_data) },
                        .alignment = ice::align_of<ice::ecs::EntityOperations>
                    }
                );
                allocation_node = it;
            }
        }

        _allocator.deallocate(
            ice::Memory{
                .location = allocation_node,
                .size = { reinterpret_cast<ice::usize::base_type>(allocation_node->component_data) },
                .alignment = ice::align_of<ice::ecs::EntityOperations>
            }
        );

        EntityOperationData* data_it = _data_nodes;
        while (data_it != nullptr)
        {
            EntityOperationData* next = data_it->next;
            _allocator.deallocate(
                ice::Memory{
                    .location = data_it,
                    .size = data_it->allocated_size,
                    .alignment = ice::align_of<ice::ecs::EntityOperations::EntityOperationData>
                }
            );
            data_it = next;
        }
    }

    void EntityOperations::clear() noexcept
    {
        ice::ecs::EntityOperation* it = _root;
        ice::ecs::EntityOperation* allocation_node = it;

        while (it->next != nullptr)
        {
            it = it->next;

            if (reinterpret_cast<ice::uptr>(it->entities) == 1)
            {
                // Delete the previous known memory of nodes.
                _allocator.deallocate(
                    ice::Memory{
                        .location = allocation_node,
                        .size = { reinterpret_cast<ice::usize::base_type>(allocation_node->component_data) },
                        .alignment = ice::align_of<ice::ecs::EntityOperations>
                    }
                );
                allocation_node = it;
            }
        }

        _allocator.deallocate(
            ice::Memory{
                .location = allocation_node,
                .size = { reinterpret_cast<ice::usize::base_type>(allocation_node->component_data) },
                .alignment = ice::align_of<ice::ecs::EntityOperations>
            }
        );

        EntityOperationData* data_it = _data_nodes;
        while (data_it != nullptr)
        {
            EntityOperationData* next = data_it->next;
            _allocator.deallocate(
                ice::Memory{
                    .location = data_it,
                    .size = data_it->allocated_size,
                    .alignment = ice::align_of<ice::ecs::EntityOperations::EntityOperationData>
                }
            );
            data_it = next;
        }

        _data_nodes = detail::allocate_data_node(_allocator, 16_KiB, nullptr);
        _operations = detail::allocate_operation_nodes(_allocator, 16);
        _free_operations = ice::exchange(_operations->next, nullptr);
        _root = _operations;
    }

    void EntityOperations::grow(ice::u32 count) noexcept
    {
        ICE_ASSERT(_free_operations == nullptr, "There are still nodes available!");

        ice::ecs::EntityOperation* const new_operations = detail::allocate_operation_nodes(
            _allocator, count
        );

        _free_operations = new_operations->next;
        new_operations->next = nullptr;

        _operations->next = new_operations;
        _operations = new_operations;
    }

    auto EntityOperations::new_storage_operation() noexcept -> ice::ecs::EntityOperation*
    {
        void* dummy_variable;
        return new_storage_operation({ }, dummy_variable);
    }

    auto EntityOperations::new_storage_operation(
        ice::meminfo required_data_size,
        void*& out_operation_data_ptr
    ) noexcept -> ice::ecs::EntityOperation*
    {
        if (_free_operations == nullptr)
        {
            grow(16);
        }

        EntityOperation* free_operation = _free_operations;
        _free_operations = ice::exchange(free_operation->next, nullptr);

        _operations->next = free_operation;
        _operations = free_operation;

        if (required_data_size.size > 0_B)
        {
            // Ensure 8 bytes available in data nodes
            if (_data_nodes->available_size <= required_data_size.size)
            {
                _data_nodes->available_size = 0_B;
                _data_nodes = detail::allocate_data_node(_allocator, ice::max(required_data_size.size, 16_KiB), _data_nodes);
            }

            void* const start_ptr = _data_nodes->operation_data;
            ice::AlignResult<void*> const aligned_ptr = ice::align_to(_data_nodes->operation_data, required_data_size.alignment);

            out_operation_data_ptr = aligned_ptr.value;
            _data_nodes->operation_data = ice::ptr_add(aligned_ptr.value, required_data_size.size);
            _data_nodes->available_size = { _data_nodes->available_size.value - ice::ptr_distance(start_ptr, _data_nodes->operation_data).value };
        }
        return _operations;
    }

    auto EntityOperations::begin() const noexcept -> OperationIterator
    {
        return OperationIterator{ ._entry = _root->next };
    }

    auto EntityOperations::end() const noexcept -> OperationIterator
    {
        return OperationIterator{ ._entry = nullptr };
    }

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::Entity entity,
        ice::ecs::Archetype archetype
    ) noexcept
    {
        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(ice::meminfo_of<ice::ecs::EntityHandle>, handle_loc);
        operation->archetype = archetype;
        operation->entities = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        *operation->entities = detail::make_empty_entity_handle(entity);
        operation->entity_count = 1;
        operation->notify_entity_changes = true;
        operation->component_data = nullptr;
        operation->component_data_size = 0;
    }

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        bool notify_changes /*= false*/
    ) noexcept
    {
        ice::u32 const entity_count = ice::count(entities);

        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(ice::meminfo_of<ice::ecs::EntityHandle> * entity_count, handle_loc);
        operation->archetype = archetype;
        operation->entities = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        operation->entity_count = entity_count;
        operation->notify_entity_changes = notify_changes;
        operation->component_data = nullptr;
        operation->component_data_size = 0;

        for (ice::u32 idx = 0; idx < entity_count; ++idx)
        {
            operation->entities[idx] = detail::make_empty_entity_handle(entities[idx]);
        }
    }

    void queue_set_archetype_with_data(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::Entity const> entities,
        ice::ecs::Archetype archetype,
        ice::ecs::EntityOperations::ComponentInfo component_info,
        ice::Span<ice::Data> component_data,
        bool notify_changes
    ) noexcept
    {
        ice::ucount const entity_count = ice::count(entities);
        ice::ucount const component_count = ice::count(component_info.names);

        ice::meminfo additional_data_size = ice::meminfo_of<ice::ecs::EntityHandle> * entity_count;
        additional_data_size += ice::meminfo_of<ice::ecs::EntityOperations::ComponentInfo>;
        additional_data_size += ice::meminfo_of<ice::StringID const> * component_count;
        additional_data_size += ice::meminfo_of<ice::u32 const> * component_count;
        additional_data_size += ice::meminfo_of<ice::u32 const> * component_count;

        // Component data
        for (ice::Data const& data : component_data)
        {
            additional_data_size += { data.size, data.alignment };
        }

        // Setup the operation
        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(additional_data_size, handle_loc);

        // Set entity handles
        ice::ecs::EntityHandle* entities_ptr = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        handle_loc = entities_ptr + entity_count;

        for (ice::u32 idx = 0; idx < entity_count; ++idx)
        {
            entities_ptr[idx] = detail::make_empty_entity_handle(entities[idx]);
        }

        // Set component info object
        ice::StringID* names_ptr = reinterpret_cast<ice::StringID*>(handle_loc);
        ice::memcpy(names_ptr, ice::span::data(component_info.names), ice::span::size_bytes(component_info.names));
        handle_loc = ice::ptr_add(handle_loc, ice::span::size_bytes(component_info.names));

        ice::u32* sizes_ptr = reinterpret_cast<ice::u32*>(handle_loc);
        ice::memcpy(sizes_ptr, ice::span::data(component_info.sizes), ice::span::size_bytes(component_info.sizes));
        handle_loc = ice::ptr_add(handle_loc, ice::span::size_bytes(component_info.sizes));

        ice::u32* offsets_ptr = reinterpret_cast<ice::u32*>(handle_loc);
        ice::memcpy(offsets_ptr, ice::span::data(component_info.offsets), ice::span::size_bytes(component_info.offsets));
        handle_loc = ice::ptr_add(handle_loc, ice::span::size_bytes(component_info.offsets));

        ice::ecs::EntityOperations::ComponentInfo* component_info_ptr = reinterpret_cast<ice::ecs::EntityOperations::ComponentInfo*>(handle_loc);
        handle_loc = component_info_ptr + 1;

        component_info_ptr->names = ice::Span<ice::StringID const>{ names_ptr, component_count };
        component_info_ptr->sizes = ice::Span<ice::u32 const>{ sizes_ptr, component_count };
        component_info_ptr->offsets = ice::Span<ice::u32 const>{ offsets_ptr, component_count };

        // Copy over component data
        void const* const data_beg = handle_loc;

        ice::u32 component_idx = 0;
        for (ice::Data const& data : component_data)
        {
            // Align target pointer
            handle_loc = ice::align_to(handle_loc, data.alignment).value;

            // Copy over the data
            ice::memcpy(handle_loc, data.location, data.size);

            // Save the offset
            offsets_ptr[component_idx] = ice::ucount(ice::ptr_distance(data_beg, handle_loc).value);

            handle_loc = ice::ptr_add(handle_loc, data.size);
            component_idx += 1;
        }

        operation->archetype = archetype;
        operation->entities = entities_ptr;
        operation->entity_count = entity_count;
        operation->notify_entity_changes = notify_changes;
        operation->component_data = component_info_ptr;
        operation->component_data_size = ice::ucount(ice::ptr_distance(component_info_ptr, handle_loc).value);
    }

    void queue_remove_entity(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::EntityHandle entity
    ) noexcept
    {
        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(ice::meminfo_of<ice::ecs::EntityHandle>, handle_loc);
        operation->archetype = ice::ecs::Archetype::Invalid;
        operation->entities = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        *operation->entities = entity;
        operation->entity_count = 1;
        operation->notify_entity_changes = true;
        operation->component_data = nullptr;
        operation->component_data_size = 0;
    }

    void queue_batch_remove_entities(
        ice::ecs::EntityOperations& entity_operations,
        ice::Span<ice::ecs::EntityHandle const> entities
    ) noexcept
    {
        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(ice::meminfo_of<ice::ecs::EntityHandle> * ice::count(entities), handle_loc);
        operation->archetype = ice::ecs::Archetype::Invalid;
        operation->entities = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        ice::memcpy(operation->entities, ice::span::data(entities), ice::span::size_bytes(entities));
        operation->entity_count = ice::count(entities);
        operation->notify_entity_changes = false;
        operation->component_data = nullptr;
        operation->component_data_size = 0;
    }

    void queue_batch_remove_entities(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::QueryProvider const& query_provider,
        ice::Span<ice::ecs::Entity const> entities
    ) noexcept
    {
        if (ice::span::empty(entities))
        {
            return;
        }

        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(ice::meminfo_of<ice::ecs::EntityHandle> * ice::count(entities), handle_loc);
        operation->archetype = ice::ecs::Archetype::Invalid;
        operation->entities = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        operation->entity_count = ice::count(entities);
        operation->notify_entity_changes = false;
        operation->component_data = nullptr;
        operation->component_data_size = 0;

        // Resolve the entities to be removed.
        query_provider.resolve_entities(entities, { operation->entities, operation->entity_count });
    }

} // namespace ice::ecs
