#include "iceshard_runner.hxx"
#include "iceshard_frame.hxx"
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 FrameAllocatorCapacity = 256u * 1024u * 1024u;

    } // namespace detail

    IceshardEngineRunner::IceshardEngineRunner(ice::Allocator& alloc) noexcept
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
    {
        _previous_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[0]
        );
        _current_frame = ice::make_unique<ice::IceshardMemoryFrame>(
            _allocator,
            _frame_data_allocator[1]
        );
    }

    IceshardEngineRunner::~IceshardEngineRunner() noexcept
    {
        _current_frame = nullptr;
        _previous_frame = nullptr;
    }

    auto IceshardEngineRunner::clock() const noexcept -> ice::Clock const&
    {
        return _clock;
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

        // Reset the frame allocator inner pointers.
        [[maybe_unused]]
        bool const discarded_memory = _frame_data_allocator[_next_free_allocator].reset_and_discard();
        ICE_ASSERT(discarded_memory == false, "Memory was discarded during frame allocator reset!");

        ice::clock::update(_clock);

        _current_frame = ice::make_unique<IceshardMemoryFrame>(
            _frame_allocator,
            _frame_data_allocator[_next_free_allocator]
        );

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= ice::size(_frame_data_allocator);
    }

} // namespace ice
