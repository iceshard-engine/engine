#include "allocators/default_allocator.hxx"

#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>


namespace ice::memory
{

    struct MemoryGlobalsTable
    {
        static constexpr ice::u32 Constant_GlobalAllocatorBuffer = sizeof(ice::memory::DefaultAllocator) + alignof(ice:: memory::ScratchAllocator)
            + sizeof(ice::memory::ScratchAllocator);

        ice::u8 _buffer[Constant_GlobalAllocatorBuffer]{ 0 };

        ice::memory::DefaultAllocator* _default_allocator{ nullptr };
        ice::memory::ScratchAllocator* _default_scratch_allocator{ nullptr };

        ice::memory::ProxyAllocator* _stats_allocator{ nullptr };
        ice::Allocator* _global_allocator{ nullptr };
    };

    MemoryGlobalsTable internal_memory_globals;

    void init(ice::u32 temporary_memory) noexcept
    {
        void* ptr = internal_memory_globals._buffer;
        internal_memory_globals._default_allocator = new (ptr) ice::memory::DefaultAllocator{ };
        internal_memory_globals._global_allocator = internal_memory_globals._default_allocator;

        ptr = ice::memory::ptr_add(ptr, sizeof(ice::memory::DefaultAllocator));
        ptr = ice::memory::ptr_align_forward(ptr, alignof(ice::memory::ScratchAllocator));
        
        internal_memory_globals._default_scratch_allocator = new (ptr) ice::memory::ScratchAllocator{
            *internal_memory_globals._default_allocator,
            temporary_memory
        };

        if constexpr (ice::build::is_debug || ice::build::is_develop)
        {
            internal_memory_globals._stats_allocator = internal_memory_globals._default_allocator->make<ice::memory::ProxyAllocator>(
                *internal_memory_globals._default_allocator,
                "global"
            );

            internal_memory_globals._global_allocator = internal_memory_globals._stats_allocator;
        }
    }

    auto default_allocator() noexcept -> ice::Allocator&
    {
        return *internal_memory_globals._global_allocator;
    }

    auto default_scratch_allocator() noexcept -> ice::Allocator&
    {
        return *internal_memory_globals._default_scratch_allocator;
    }

    void shutdown() noexcept
    {
        internal_memory_globals._global_allocator = nullptr;

        if constexpr (ice::build::is_debug || ice::build::is_develop)
        {
            internal_memory_globals._stats_allocator->~ProxyAllocator();
        }

        internal_memory_globals._default_allocator->deallocate(internal_memory_globals._stats_allocator);
        internal_memory_globals._default_scratch_allocator->~ScratchAllocator();
        internal_memory_globals._default_allocator->~DefaultAllocator();
        internal_memory_globals = MemoryGlobalsTable{ };
    }

} // namespace ice::memory
