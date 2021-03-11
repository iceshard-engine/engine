#include "iceshard_frame.hxx"

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 MiB = 1024 * 1024;
        static constexpr ice::u32 InputsAllocatorCapacity = 4 * MiB;
        static constexpr ice::u32 MessageAllocatorCapacity = 20 * MiB;
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
        , _message_allocator{ _allocator, detail::MessageAllocatorCapacity }
        , _storage_allocator{ _allocator, detail::StorageAllocatorCapacity }
        , _data_allocator{ _allocator, detail::DataAllocatorCapacity }
    {
        detail::global_frame_counter += 1;
    }

    auto IceshardMemoryFrame::memory_consumption() noexcept -> ice::u32
    {
        return sizeof(IceshardMemoryFrame)
            + _inputs_allocator.total_allocated()
            + _message_allocator.total_allocated()
            + _storage_allocator.total_allocated()
            + _data_allocator.total_allocated();
    }

} // namespace ice
