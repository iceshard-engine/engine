#include "iceshard_runner.hxx"
#include "iceshard_engine.hxx"
#include "iceshard_frame.hxx"

#include "world/iceshard_world.hxx"
#include "world/iceshard_world_manager.hxx"

#include "gfx/iceshard_gfx_pass.hxx"

#include <ice/input/input_tracker.hxx>
#include <ice/assert.hxx>


namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 FrameAllocatorCapacity = 256u * 1024u * 1024u;
        static constexpr ice::u32 GfxFrameAllocatorCapacity = 16u * 1024u * 1024u;

    } // namespace detail

    IceshardEngineRunner::IceshardEngineRunner(
        ice::Allocator& alloc,
        ice::IceshardEngine& engine,
        ice::IceshardWorldManager& world_manager,
        ice::UniquePtr<ice::input::InputTracker> input_tracker,
        ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device
    ) noexcept
        : ice::EngineRunner{ }
        , _allocator{ alloc, "engine-runner" }
        , _engine{ engine }
        , _clock{ ice::clock::create_clock() }
        , _thread_pool{ ice::create_simple_threadpool(_allocator, 4) }
        , _graphics_thread{ ice::create_task_thread(_allocator) }
        , _frame_allocator{ _allocator, "frame-allocator" }
        , _frame_data_allocator{
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity, "frame-alloc-1" },
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity, "frame-alloc-2" }
        }
        , _frame_gfx_allocator{
            ice::memory::ProxyAllocator{ _frame_allocator, "gfx-frame-alloc-1" },
            ice::memory::ProxyAllocator{ _frame_allocator, "gfx-frame-alloc-2" }
        }
        , _next_free_allocator{ 0 }
        , _previous_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _current_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _world_manager{ world_manager }
        , _world_tracker{ _allocator }
        , _input_tracker{ ice::move(input_tracker) }
        , _gfx_device{ ice::move(gfx_device) }
        , _gfx_current_frame{ ice::make_unique_null<ice::gfx::IceGfxFrame>() }
        , _next_tasks{ _allocator }
        , _gfx_next_tasks{ _allocator }
    {
        _previous_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[0]
        );
        _current_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[1]
        );

        _gfx_current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator, _frame_gfx_allocator[1]
        );

        _next_tasks.reserve(100);
        _gfx_next_tasks.reserve(100);

        activate_worlds();
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
         _graphics_thread->stop();
         _graphics_thread->join();
         _graphics_thread = nullptr;
         _thread_pool = nullptr;

        deactivate_worlds();

        _gfx_current_frame = nullptr;

        _gfx_device = nullptr;
        _current_frame = nullptr;
        _previous_frame = nullptr;
    }

    auto IceshardEngineRunner::clock() const noexcept -> ice::Clock const&
    {
        return _clock;
    }

    auto IceshardEngineRunner::input_tracker() noexcept -> ice::input::InputTracker&
    {
        return *_input_tracker;
    }

    void IceshardEngineRunner::process_device_queue(
        ice::input::DeviceQueue const& device_queue
    ) noexcept
    {
        _input_tracker->process_device_queue(device_queue, _current_frame->input_events());
    }

    auto IceshardEngineRunner::thread_pool() noexcept -> ice::TaskThreadPool&
    {
        return *_thread_pool;
    }

    auto IceshardEngineRunner::graphics_device() noexcept -> ice::gfx::GfxDevice&
    {
        return *_gfx_device;
    }

    auto IceshardEngineRunner::graphics_frame() noexcept -> ice::gfx::GfxFrame&
    {
        return *_gfx_current_frame;
    }

    auto IceshardEngineRunner::previous_frame() const noexcept -> EngineFrame const&
    {
        return *_previous_frame;
    }

    auto IceshardEngineRunner::current_frame() const noexcept -> EngineFrame const&
    {
        return *_current_frame;
    }

    auto IceshardEngineRunner::current_frame() noexcept -> EngineFrame&
    {
        return *_current_frame;
    }

    void IceshardEngineRunner::next_frame() noexcept
    {
        // [issue #??] We cannot move this wait lower as for now we are still accessing a single GfxPass object from both the Gfx and Runtim threads.
        _graphics_thread_event.wait();
        _graphics_thread_event.reset();

        // Update all active worlds
        _world_tracker.update_active_worlds(*this);

        // Move the current frame to the 'previous' slot.
        _previous_frame = ice::move(_current_frame);
        _previous_frame->start_all();

        _graphics_thread->schedule(
            graphics_task(
                ice::move(_gfx_current_frame),
                &_graphics_thread_event
            )
        );

        // Reset the frame allocator inner pointers.
        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        _gfx_current_frame = ice::make_unique<ice::gfx::IceGfxFrame>(
            _allocator,
            _frame_gfx_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);

        // Handle requests for the next frame
        for (ice::EngineRequest const& request : _previous_frame->requests())
        {
            switch (request.name.hash_value)
            {
            case ice::stringid_hash(Request_ActivateWorld):
                _world_tracker.activate_world(
                    _engine, *this,
                    static_cast<ice::IceshardWorld*>(
                        reinterpret_cast<ice::World*>(request.payload)
                    )
                );
                break;
            case ice::stringid_hash(Request_DeactivateWorld):
                _world_tracker.deactivate_world(
                    _engine, *this,
                    static_cast<ice::IceshardWorld*>(
                        reinterpret_cast<ice::World*>(request.payload)
                    )
                );
                break;
            default:
                break;
            }
        }
    }

    auto IceshardEngineRunner::graphics_task(
        ice::UniquePtr<ice::gfx::IceGfxFrame> gfx_frame,
        ice::ManualResetEvent* reset_event
    ) noexcept -> ice::Task<>
    {
        ice::u32 const image_index = _gfx_device->next_frame();
        ice::gfx::IceGfxQueueGroup& queue_group = _gfx_device->queue_group(image_index);

        gfx_frame->start_all();
        gfx_frame->execute_passes(previous_frame(), queue_group);
        gfx_frame->wait_ready();
        _previous_frame->wait_ready();

        _gfx_device->present(image_index);

        reset_event->set();
        co_return;
    }

    void IceshardEngineRunner::activate_worlds() noexcept
    {
        for (auto const& entry : _world_manager.worlds())
        {
            _world_tracker.activate_world(_engine, *this, entry.value);
        }
    }

    void IceshardEngineRunner::deactivate_worlds() noexcept
    {
        for (auto const& entry : _world_manager.worlds())
        {
            _world_tracker.deactivate_world(_engine, *this, entry.value);
        }
    }

} // namespace ice
