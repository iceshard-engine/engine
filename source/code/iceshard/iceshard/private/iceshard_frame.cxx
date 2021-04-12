#include "iceshard_frame.hxx"
#include <ice/engine_request.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 KiB = 1024;
        static constexpr ice::u32 MiB = 1024 * KiB;

        static constexpr ice::u32 InputsAllocatorCapacity = 4 * MiB;
        static constexpr ice::u32 RequestAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 MessageAllocatorCapacity = 19 * MiB;
        static constexpr ice::u32 StorageAllocatorCapacity = 8 * MiB;
        static constexpr ice::u32 DataAllocatorCapacity = 196 * MiB;

        static ice::u32 global_frame_counter = 0;

    } // namespace detail

    IceshardMemoryFrame::IceshardMemoryFrame(
        ice::memory::ScratchAllocator& alloc
    ) noexcept
        : ice::EngineFrame{ }
        , _allocator{ alloc }
        , _inputs_allocator{ _allocator, detail::InputsAllocatorCapacity }
        , _request_allocator{ _allocator, detail::RequestAllocatorCapacity }
        , _message_allocator{ _allocator, detail::MessageAllocatorCapacity }
        , _storage_allocator{ _allocator, detail::StorageAllocatorCapacity }
        , _data_allocator{ _allocator, detail::DataAllocatorCapacity }
        , _requests{ _request_allocator }
    {
        detail::global_frame_counter += 1;

        ice::pod::array::reserve(_requests, (detail::RequestAllocatorCapacity / sizeof(ice::EngineRequest)) - 10);
    }

    IceshardMemoryFrame::~IceshardMemoryFrame() noexcept
    {
    }

    auto IceshardMemoryFrame::memory_consumption() noexcept -> ice::u32
    {
        return sizeof(IceshardMemoryFrame)
            + _inputs_allocator.total_allocated()
            + _request_allocator.total_allocated()
            + _message_allocator.total_allocated()
            + _storage_allocator.total_allocated()
            + _data_allocator.total_allocated();
    }

    void IceshardMemoryFrame::push_requests(
        ice::Span<EngineRequest const> requests
    ) noexcept
    {
        ice::pod::array::push_back(_requests, requests);
    }

    auto IceshardMemoryFrame::requests() const noexcept -> ice::Span<EngineRequest const>
    {
        return _requests;
    }

} // namespace ice
