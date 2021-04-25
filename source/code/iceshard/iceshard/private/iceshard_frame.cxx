#include "iceshard_frame.hxx"
#include <ice/task_sync_wait.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/engine_request.hxx>
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
        static constexpr ice::u32 DataAllocatorCapacity = 224 * MiB;

        static ice::u32 global_frame_counter = 0;

    } // namespace detail

    IceshardMemoryFrame::IceshardMemoryFrame(
        ice::memory::ScratchAllocator& alloc
    ) noexcept
        : ice::EngineFrame{ }
        , _allocator{ alloc }
        , _inputs_allocator{ _allocator, detail::InputsAllocatorCapacity }
        , _request_allocator{ _allocator, detail::RequestAllocatorCapacity }
        , _tasks_allocator{ _allocator, detail::TaskAllocatorCapacity }
        , _storage_allocator{ _allocator, detail::StorageAllocatorCapacity }
        , _data_allocator{ _allocator, detail::DataAllocatorCapacity }
        , _input_events{ _inputs_allocator }
        , _requests{ _request_allocator }
        , _named_objects{ _storage_allocator }
        , _frame_tasks{ _tasks_allocator }
    {
        detail::global_frame_counter += 1;

        ice::pod::array::reserve(_input_events, (detail::InputsAllocatorCapacity / sizeof(ice::input::InputEvent)) - 10);
        ice::pod::array::reserve(_requests, (detail::RequestAllocatorCapacity / sizeof(ice::EngineRequest)) - 10);
        ice::pod::hash::reserve(_named_objects, (detail::StorageAllocatorCapacity / (sizeof(ice::pod::Hash<ice::uptr>::Entry) + sizeof(ice::u32))) - 10);

        _frame_tasks.reserve(detail::TaskAllocatorCapacity / sizeof(ice::Task<void>) - 10);
    }

    IceshardMemoryFrame::~IceshardMemoryFrame() noexcept
    {
        for (auto const& entry : _named_objects)
        {
            _data_allocator.deallocate(entry.value);
        }
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

    void IceshardMemoryFrame::wait_ready() noexcept
    {
        if (_frame_tasks.empty() == false)
        {
            ice::u32 const task_count = static_cast<ice::u32>(_frame_tasks.size());

            // [issue #36] There is currently an issue that prevents to use a std::pmr::vector for
            //  any object containing a std::atomic<T> member.
            //  This issue exists for both the standard vector version and the Iceshard version, `ice::Vector`.
            //  Because of this we allocate enough memory, constructor and pass the needed array of ManualResetEvents manualy.
            void* ptr = _allocator.allocate(task_count * sizeof(ManualResetEvent) + 8);
            void* aligned_ptr = ice::memory::ptr_align_forward(ptr, alignof(ManualResetEvent));

            // Call for each event the default constructor.
            ManualResetEvent* ptr_events = reinterpret_cast<ManualResetEvent*>(aligned_ptr);
            for (ice::u32 idx = 0; idx < task_count; ++idx)
            {
                new (reinterpret_cast<void*>(ptr_events + idx)) ManualResetEvent{ };
            }

            // [issue #35] Wait for all frame tasks to finish.
            //  Currently the `sync_wait_all` calls the .wait() method on all events.
            //  We should probably re-think the whole API.
            ice::sync_wait_all(
                _allocator,
                _frame_tasks,
                { ptr_events, task_count }
            );

            _frame_tasks.clear();

            // Call for each event their destructor.
            for (ice::u32 idx = 0; idx < task_count; ++idx)
            {
                (ptr_events + idx)->~ManualResetEvent();
            }

            // Deallocate the memory.
            _allocator.deallocate(ptr);
        }
    }

    void IceshardMemoryFrame::push_requests(
        ice::Span<EngineRequest const> requests
    ) noexcept
    {
        ice::pod::array::push_back(_requests, requests);
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

    auto IceshardMemoryFrame::requests() const noexcept -> ice::Span<EngineRequest const>
    {
        return _requests;
    }

} // namespace ice
