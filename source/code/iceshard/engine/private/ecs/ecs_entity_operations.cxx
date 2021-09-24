#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    struct EntityOperations::EntityOperationData
    {
        EntityOperationData* next;

        void* operation_data;
        ice::u32 available_data_size;
    };

    namespace detail
    {

        auto allocate_operation_nodes(
            ice::Allocator& alloc,
            ice::u32 count
        ) noexcept -> ice::ecs::EntityOperation*
        {
            void* const node_data = alloc.allocate(
                sizeof(ice::ecs::EntityOperation) * count + 1,
                alignof(ice::ecs::EntityOperation)
            );

            EntityOperation* operations = reinterpret_cast<EntityOperation*>(node_data);
            for (ice::u32 idx = 1; idx < count; ++idx)
            {
                operations[idx - 1].next = operations + idx;
            }
            operations[15].next = nullptr;

            // Make the first operation empty and mark it as an allocation guard node.
            operations->entity_count = 0;
            operations->component_data_size = 0;
            operations->archetype = Archetype::Invalid;
            operations->entities = reinterpret_cast<ice::ecs::EntityHandle*>(1); // Allocation guard node
            operations->component_data = nullptr;

            return operations;
        }

        auto allocate_data_node(
            ice::Allocator& alloc,
            ice::u32 data_block_size,
            ice::ecs::EntityOperations::EntityOperationData* previous_node
        ) noexcept -> ice::ecs::EntityOperations::EntityOperationData*
        {
            void* const data_block = alloc.allocate(
                sizeof(ice::ecs::EntityOperations::EntityOperationData) + data_block_size,
                alignof(ice::ecs::EntityOperations::EntityOperationData)
            );

            EntityOperations::EntityOperationData* data_node = reinterpret_cast<EntityOperations::EntityOperationData*>(data_block);
            data_node->available_data_size = data_block_size;
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
        , _data_nodes{ detail::allocate_data_node(_allocator, 1024 * 16, nullptr) }
    {
        ice::ecs::EntityOperation* const new_operations = detail::allocate_operation_nodes(
            _allocator, initial_count
        );

        _root = new_operations;
        _operations = new_operations;
        _free_operations = _operations->next;
        _operations->next = nullptr;

        ice::ecs::EntityOperation* const new_data_node = detail::allocate_operation_nodes(
            _allocator, initial_count
        );
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
                _allocator.deallocate(allocation_node);
                allocation_node = it;
            }
        }

        _allocator.deallocate(allocation_node);
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
        return new_storage_operation(0, dummy_variable);
    }

    auto EntityOperations::new_storage_operation(
        ice::u32 required_data_size,
        void*& out_operation_data_ptr
    ) noexcept -> ice::ecs::EntityOperation*
    {
        if (_free_operations == nullptr)
        {
            grow(16);
        }

        EntityOperation* free_operation = _free_operations;
        _free_operations = free_operation->next;
        free_operation->next = nullptr;

        _operations->next = free_operation;
        _operations = free_operation;

        if (required_data_size > 0)
        {
            // Ensure 8 bytes available in data nodes
            if (_data_nodes->available_data_size <= required_data_size)
            {
                _data_nodes->available_data_size = 0;
                _data_nodes = detail::allocate_data_node(_allocator, ice::max(required_data_size, 1024 * 16u), _data_nodes);
            }

            out_operation_data_ptr = ice::memory::ptr_align_forward(_data_nodes->operation_data, 8);

            _data_nodes->available_data_size -= ice::memory::ptr_distance(
                _data_nodes->operation_data,
                out_operation_data_ptr
            );
        }

        return _operations;
    }

    void EntityOperations::set_archetype(ice::ecs::EntityHandle entity, ice::ecs::Archetype archetype) noexcept
    {
        if (_free_operations == nullptr)
        {
            grow(16);
        }

        EntityOperation* free_operation = _free_operations;
        _free_operations = free_operation->next;
        free_operation->next = nullptr;

        _operations->next = free_operation;
        _operations = free_operation;

        _operations->archetype = archetype;
        _operations->entity_count = 1;

        // Ensure 8 bytes available in data nodes
        if (_data_nodes->available_data_size <= sizeof(ice::ecs::EntityHandle))
        {
            _data_nodes->available_data_size = 0;
            _data_nodes = detail::allocate_data_node(_allocator, 1024 * 16, _data_nodes);
        }

        _operations->entities = reinterpret_cast<EntityHandle*>(
            ice::memory::ptr_align_forward(_data_nodes->operation_data, 8)
        );

        _data_nodes->available_data_size -= ice::memory::ptr_distance(
            _data_nodes->operation_data,
            _operations->entities
        );

        // Assign the one entity into the array.
        *_operations->entities = entity;
        _operations->component_data = nullptr;
        _operations->component_data_size = 0;
    }

    void queue_set_archetype(
        ice::ecs::EntityOperations& entity_operations,
        ice::ecs::EntityHandle entity,
        ice::ecs::Archetype archetype
    ) noexcept
    {
        void* handle_loc;
        EntityOperation* operation = entity_operations.new_storage_operation(sizeof(ice::ecs::EntityHandle), handle_loc);
        operation->archetype = archetype;
        operation->entities = reinterpret_cast<ice::ecs::EntityHandle*>(handle_loc);
        *operation->entities = entity;
        operation->entity_count = 1;
    }

} // namespace ice::ecs
