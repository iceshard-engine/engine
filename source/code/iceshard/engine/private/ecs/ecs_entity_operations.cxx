/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
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
        while (_entry != nullptr && _entry->entities == reinterpret_cast<Entity const*>(1))
        {
            _entry = _entry->next;
        }
        return *this;
    }

    namespace detail
    {

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
            operations->entities = reinterpret_cast<ice::ecs::Entity*>(1); // Allocation guard node
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
        ice::ecs::EntityIndex& entities,
        ice::ecs::ArchetypeIndex const& archetypes,
        ice::u32 initial_count
    ) noexcept
        : _allocator{ alloc }
        , _entities{ entities }
        , _archetypes{ archetypes }
        , _root{ nullptr }
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

    void queue_remove_entity(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::Entity entity
    ) noexcept
    {
        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(ice::meminfo_of<ice::ecs::Entity>, handle_loc);
        operation->archetype = ice::ecs::Archetype::Invalid;
        operation->entities = reinterpret_cast<ice::ecs::Entity*>(handle_loc);
        operation->entities[0] = entity;
        operation->entity_count = 1;
        operation->component_data = nullptr;
        operation->component_data_size = 0;
    }

    void EntityOperations::destroy(ice::Span<ice::ecs::Entity const> entities) noexcept
    {
        if (entities.is_empty())
        {
            return;
        }

        void* handle_loc;
        EntityOperation* operation = new_storage_operation(ice::meminfo_of<ice::ecs::Entity> * entities.size().u32(), handle_loc);
        operation->archetype = ice::ecs::Archetype::Invalid;
        operation->entities = reinterpret_cast<ice::ecs::Entity*>(handle_loc);
        operation->entity_count = entities.size().u32();
        operation->component_data = nullptr;
        operation->component_data_size = 0;

        ice::memcpy(operation->entities, ice::span::data(entities), ice::span::size_bytes(entities));
    }

    auto OperationBuilder::with_data(
        ice::ecs::OperationComponentInfo component_info,
        ice::Span<ice::Data const> component_data
    ) noexcept -> Result
    {
        if (entities.is_empty() || (mode == 2 && index_create_count == 0) || mode == 0)
        {
            return Result{ *this };
        }

        ice::u32 const entity_count = mode == 2 ? index_create_count : entities.size().u32();
        ice::u32 const component_count = component_info.names.size().u32();

        ice::meminfo additional_data_size = ice::meminfo{ filter_data_size, ice::ualign::b_8 };
        additional_data_size += ice::meminfo_of<ice::ecs::Entity> * entity_count;

        // Data for storing component info
        additional_data_size += ice::meminfo_of<ice::ecs::OperationComponentInfo>;
        additional_data_size.size += ice::span::size_bytes(component_info.names);
        additional_data_size.size += ice::span::size_bytes(component_info.sizes);
        additional_data_size.size += ice::span::size_bytes(component_info.offsets);

        // Component data
        for (ice::Data const& data : component_data)
        {
            additional_data_size += { data.size, data.alignment };
        }

        void* operation_data = nullptr;
        ice::ecs::EntityOperation* operation = operations.new_storage_operation(
            additional_data_size,
            operation_data
        );

        ice::memcpy(operation_data, filter_data, ice::usize{ filter_data_size });
        void const* filter_ptr = operation_data;
        operation_data = ice::ptr_add(operation_data, filter_data_size);

        ice::ecs::Entity* entities_ptr = reinterpret_cast<ice::ecs::Entity*>(operation_data);
        if (mode == 1)
        {
            ice::memcpy(entities_ptr, ice::span::data(entities), ice::span::size_bytes(entities));
        }
        else
        {
            ICE_ASSERT_CORE(mode == 2);
            index->create_many({ entities_ptr, index_create_count });
        }

        // Set component info object
        ice::StringID* names_ptr = reinterpret_cast<ice::StringID*>(entities_ptr + entity_count);
        ice::memcpy(names_ptr, ice::span::data(component_info.names), ice::span::size_bytes(component_info.names));

        ice::u32* sizes_ptr = reinterpret_cast<ice::u32*>(names_ptr + component_count);
        ice::memcpy(sizes_ptr, ice::span::data(component_info.sizes), ice::span::size_bytes(component_info.sizes));

        ice::u32* offsets_ptr = reinterpret_cast<ice::u32*>(sizes_ptr + component_count);

        // We update now the operation data pointer to where we store the component info object.
        //  We will calculate data offsets from here too.
        operation_data = offsets_ptr + component_count;

        // Set the component info object with the above pointers.
        OperationComponentInfo* component_info_ptr;
        {
            component_info_ptr = reinterpret_cast<OperationComponentInfo*>(operation_data);
            component_info_ptr->names = ice::Span<ice::StringID const>{ names_ptr, component_count };
            component_info_ptr->sizes = ice::Span<ice::u32 const>{ sizes_ptr, component_count };
            component_info_ptr->offsets = ice::Span<ice::u32 const>{ offsets_ptr, component_count };
            operation_data = component_info_ptr + 1;
        }

        // Copy over component data
        void const* const data_beg = operation_data;

        ice::u32 component_idx = 0;
        for (ice::Data const& data : component_data)
        {
            // Align target pointer
            operation_data = ice::align_to(operation_data, data.alignment).value;

            // Copy over the data
            ice::memcpy(operation_data, data.location, data.size);

            // Save the offset
            offsets_ptr[component_idx] = ice::u32(ice::ptr_distance(data_beg, operation_data).value);

            operation_data = ice::ptr_add(operation_data, data.size);
            component_idx += 1;
        }

        operation->archetype = archetype;
        operation->entities = entities_ptr;
        operation->entity_count = entity_count;
        operation->component_data = component_info_ptr;
        operation->component_data_size = ice::u32(ice::ptr_distance(component_info_ptr, operation_data).value);
        operation->filter_data = filter_ptr;

        if (mode == 2)
        {
            entities = { entities_ptr, entity_count };
        }
        mode = 0;

        return { *this };
    }

    auto OperationBuilder::finalize() noexcept -> Result
    {
        if (mode != 0)
        {
            ice::u32 const entity_count = mode == 2 ? index_create_count : entities.size().u32();

            ice::meminfo required_memory = ice::meminfo{ filter_data_size, ice::ualign::b_8 };
            required_memory += ice::meminfo_of<ice::ecs::Entity> * entity_count;

            void* operation_data;
            EntityOperation* operation = operations.new_storage_operation(required_memory, operation_data);

            ice::memcpy(operation_data, filter_data, ice::usize{ filter_data_size });
            void const* filter_ptr = operation_data;
            operation_data = ice::ptr_add(operation_data, filter_data_size);

            ice::ecs::Entity* entities_ptr = reinterpret_cast<ice::ecs::Entity*>(operation_data);
            if (mode == 1)
            {
                ice::memcpy(entities_ptr, ice::span::data(entities), ice::span::size_bytes(entities));
            }
            else
            {
                ICE_ASSERT_CORE(mode == 2);
                index->create_many({ entities_ptr, index_create_count });
            }

            operation->archetype = archetype;
            operation->entities = entities_ptr;
            operation->entity_count = entity_count;
            operation->component_data = nullptr;
            operation->component_data_size = 0;
            operation->filter_data = filter_ptr;

            ice::memcpy(operation_data, filter_data, filter_data_size);
            ice::memcpy(operation->entities, ice::span::data(entities), ice::span::size_bytes(entities));

            if (mode == 2)
            {
                entities = { entities_ptr, entity_count };
            }
            mode = 0;
        }

        return { *this };
    }

    OperationBuilder::~OperationBuilder() noexcept
    {
        this->finalize();
    }

} // namespace ice::ecs
