#include "iceshard_runner.hxx"
#include "iceshard_frame.hxx"

#include "world/iceshard_world.hxx"
#include "world/iceshard_world_manager.hxx"

#include "gfx/iceshard_gfx_pass.hxx"

#include <ice/assert.hxx>


namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 FrameAllocatorCapacity = 256u * 1024u * 1024u;

    } // namespace detail

    IceshardEngineRunner::IceshardEngineRunner(
        ice::Allocator& alloc,
        ice::IceshardWorldManager& world_manager,
        ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device
    ) noexcept
        : ice::EngineRunner{ }
        , _allocator{ alloc }
        , _clock{ ice::clock::create_clock() }
        , _frame_allocator{ _allocator, "frame-allocator" }
        , _frame_data_allocator{
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity },
            ice::memory::ScratchAllocator{ _frame_allocator, detail::FrameAllocatorCapacity }
        }
        , _previous_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _current_frame{ ice::make_unique_null<ice::IceshardMemoryFrame>() }
        , _world_manager{ world_manager }
        , _world_tracker{ _allocator }
        , _gfx_device{ ice::move(gfx_device) }
        , _gfx_current_frame{ ice::make_unique_null<ice::gfx::IceGfxBaseFrame>() }
    {
        _previous_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[0]
        );
        _current_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[1]
        );

        _gfx_current_frame = _gfx_device->next_frame(_allocator);// ice::make_unique<ice::gfx::IceGfxBaseFrame>(_allocator);

        activate_worlds();
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
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
        // Move the current frame to the 'previous' slot.
        _previous_frame = ice::move(_current_frame);

        _gfx_current_frame->execute_passes(previous_frame());

        for (ice::EngineRequest const& request : _previous_frame->requests())
        {
            switch (request.name.hash_value)
            {
            case ice::stringid_hash(Request_ActivateWorld):
                _world_tracker.activate_world(*this,
                    static_cast<ice::IceshardWorld*>(
                        reinterpret_cast<ice::World*>(request.payload)
                    )
                );
                break;
            case ice::stringid_hash(Request_DeactivateWorld):
                _world_tracker.deactivate_world(*this,
                    static_cast<ice::IceshardWorld*>(
                        reinterpret_cast<ice::World*>(request.payload)
                    )
                );
                break;
            default:
                break;
            }
        }

        // Reset the frame allocator inner pointers.
        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);

        // Update all active worlds
        _world_tracker.update_active_worlds(*this);

        _gfx_current_frame->present();
        _gfx_current_frame = _gfx_device->next_frame(_allocator);
    }

    void IceshardEngineRunner::activate_worlds() noexcept
    {
        for (auto const& entry : _world_manager.worlds())
        {
            _world_tracker.activate_world(*this, entry.value);
        }
    }

    void IceshardEngineRunner::deactivate_worlds() noexcept
    {
        for (auto const& entry : _world_manager.worlds())
        {
            _world_tracker.deactivate_world(*this, entry.value);
        }
    }

} // namespace ice
