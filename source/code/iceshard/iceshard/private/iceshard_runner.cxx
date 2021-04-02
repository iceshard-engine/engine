#include "iceshard_runner.hxx"
#include "iceshard_frame.hxx"
#include "world/iceshard_world.hxx"
#include <ice/world/world.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 FrameAllocatorCapacity = 256u * 1024u * 1024u;

    } // namespace detail

    IceshardEngineRunner::IceshardEngineRunner(
        ice::Allocator& alloc,
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

        _gfx_current_frame = ice::make_unique<ice::gfx::IceGfxBaseFrame>(_allocator);
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
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

        _gfx_current_frame->present();

        // Reset the frame allocator inner pointers.
        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _gfx_current_frame = _gfx_device->next_frame(_allocator);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);
    }

    void IceshardEngineRunner::update_world(
        ice::World* world
    ) noexcept
    {
        world->update(*this, WorldUpdateKey{ });
    }

} // namespace ice
