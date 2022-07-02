#include "iceshard_frame.hxx"
#include "iceshard_task_executor.hxx"
#include <ice/engine_shards.hxx>
#include <ice/task_sync_wait.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 KiB = 1024;
        static constexpr ice::u32 MiB = 1024 * KiB;

        static constexpr ice::u32 InputsAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 RequestAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 TaskAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 StorageAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 DataAllocatorCapacity = 16 * MiB;

        static ice::u32 global_frame_counter = 0;

    } // namespace detail

    IceshardMemoryFrame::IceshardMemoryFrame(
        ice::memory::ScratchAllocator& alloc
    ) noexcept
        : ice::EngineFrame{ }
        , _index{ detail::global_frame_counter }
        , _allocator{ alloc }
        , _inputs_allocator{ _allocator, detail::InputsAllocatorCapacity, "inputs" }
        , _request_allocator{ _allocator, detail::RequestAllocatorCapacity, "request" }
        , _tasks_allocator{ _allocator, detail::TaskAllocatorCapacity, "tasks" }
        , _storage_allocator{ _allocator, detail::StorageAllocatorCapacity, "storage" }
        , _data_allocator{ _allocator, detail::DataAllocatorCapacity, "data" }
        , _input_events{ _inputs_allocator }
        , _shards{ _request_allocator }
        , _entity_operations{ _data_allocator } // #todo change the allocator?
        , _named_objects{ _storage_allocator }
        , _frame_tasks{ _tasks_allocator }
        , _task_executor{ _allocator, ice::Vector<ice::Task<void>>{ _allocator } }
    {
        detail::global_frame_counter += 1;

        ice::pod::array::reserve(_input_events, (detail::InputsAllocatorCapacity / sizeof(ice::input::InputEvent)) - 10);
        ice::pod::hash::reserve(_named_objects, (detail::StorageAllocatorCapacity / (sizeof(ice::pod::Hash<ice::uptr>::Entry) + sizeof(ice::u32))) - 10);
        ice::shards::reserve(_shards, (detail::RequestAllocatorCapacity / sizeof(ice::Shard)) - 10);

        _frame_tasks.reserve((detail::TaskAllocatorCapacity - 1024) / sizeof(ice::Task<void>));

        ice::shards::push_back(_shards, ice::Shard_FrameTick | _index);
    }

    IceshardMemoryFrame::~IceshardMemoryFrame() noexcept
    {
        ice::FrameEndOperationData* operation_head = _frame_end_operation;
        while (_frame_end_operation.compare_exchange_weak(operation_head, nullptr, std::memory_order::relaxed, std::memory_order::acquire) == false)
        {
            continue;
        }

        ice::EngineTaskOperationBaseData* operation = operation_head;
        while (operation != nullptr)
        {
            ice::EngineTaskOperationBaseData* next_operation = operation->next;
            operation->coroutine.resume();
            operation = next_operation;
        }

        _task_executor.wait_ready();

        for (auto const& entry : _named_objects)
        {
            _data_allocator.deallocate(entry.value);
        }
    }

    auto IceshardMemoryFrame::index() const noexcept -> ice::u32
    {
        return _index;
    }

    auto IceshardMemoryFrame::allocator() noexcept -> ice::Allocator&
    {
        return _data_allocator;
    }

    auto IceshardMemoryFrame::memory_consumption() noexcept -> ice::u32
    {
        return sizeof(IceshardMemoryFrame)
            + _inputs_allocator.total_allocated()
            + _request_allocator.total_allocated()
            + _tasks_allocator.total_allocated()
            + _storage_allocator.total_allocated()
            + _data_allocator.total_allocated();
    }

    auto IceshardMemoryFrame::input_events() noexcept -> ice::pod::Array<ice::input::InputEvent>&
    {
        return _input_events;
    }

    auto IceshardMemoryFrame::input_events() const noexcept -> ice::Span<ice::input::InputEvent const>
    {
        return _input_events;
    }

    void IceshardMemoryFrame::execute_task(ice::Task<void> task) noexcept
    {
        // Adds a task to be executed later during the frame update.
        _frame_tasks.push_back(ice::move(task));
    }

    void IceshardMemoryFrame::start_all() noexcept
    {
        _task_executor = IceshardTaskExecutor{ _allocator, ice::move(_frame_tasks) };
        _task_executor.start_all();
    }

    void IceshardMemoryFrame::wait_ready() noexcept
    {
        _task_executor.wait_ready();
    }

    auto IceshardMemoryFrame::shards() noexcept -> ice::ShardContainer&
    {
        return _shards;
    }

    auto IceshardMemoryFrame::shards() const noexcept -> ice::ShardContainer const&
    {
        return _shards;
    }

    auto IceshardMemoryFrame::entity_operations() noexcept -> ice::ecs::EntityOperations&
    {
        return _entity_operations;
    }

    auto IceshardMemoryFrame::entity_operations() const noexcept -> ice::ecs::EntityOperations const&
    {
        return _entity_operations;
    }

    auto IceshardMemoryFrame::named_data(
        ice::StringID_Arg name
    ) noexcept -> void*
    {
        return ice::pod::hash::get(_named_objects, ice::hash(name), nullptr);
    }

    auto IceshardMemoryFrame::named_data(
        ice::StringID_Arg name
    ) const noexcept -> void const*
    {
        return ice::pod::hash::get(_named_objects, ice::hash(name), nullptr);
    }

    auto IceshardMemoryFrame::allocate_named_data(
        ice::StringID_Arg name,
        ice::u32 size,
        ice::u32 alignment
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_objects, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        void* object_ptr = _data_allocator.allocate(size, alignment);
        ice::pod::hash::set(_named_objects, name_hash, object_ptr);
        return object_ptr;
    }

    auto IceshardMemoryFrame::allocate_named_array(
        ice::StringID_Arg name,
        ice::u32 element_size,
        ice::u32 alignment,
        ice::u32 count
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_objects, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        // TODO: To be refactored with the planned introduction of a proper 'size' type.
        ICE_ASSERT(alignment >= sizeof(ice::u32), "Cannot store array size in fron of the array!");

        void* object_ptr = _data_allocator.allocate(alignment + element_size * count, alignment);
        *reinterpret_cast<ice::u32*>(object_ptr) = count;
        ice::pod::hash::set(_named_objects, name_hash, object_ptr);
        return object_ptr;
    }

    void IceshardMemoryFrame::release_named_data(ice::StringID_Arg name) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_objects, name_hash) == true,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        void* data = ice::pod::hash::get<void*>(_named_objects, name_hash, nullptr);
        _allocator.deallocate(data);
        ice::pod::hash::set<void*>(_named_objects, name_hash, nullptr);
    }

    auto IceshardMemoryFrame::schedule_frame_end() noexcept -> ice::FrameEndOperation
    {
        return { *this };
    }

    void IceshardMemoryFrame::schedule_internal(ice::FrameEndOperationData& operation) noexcept
    {
        ice::FrameEndOperationData* expected_head = _frame_end_operation.load(std::memory_order_acquire);

        do
        {
            operation.next = expected_head;
        }
        while (
            _frame_end_operation.compare_exchange_weak(
                expected_head,
                &operation,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceshardMemoryFrame::schedule_query_internal(ice::ecs::ScheduledQueryData& query_data) noexcept
    {
        ice::ecs::ScheduledQueryData* expected_head = _query_operation.load(std::memory_order_acquire);

        do
        {
            query_data.next = expected_head;
        } while (
            _query_operation.compare_exchange_weak(
                expected_head,
                &query_data,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

} // namespace ice
