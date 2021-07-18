#include "allocators/default_allocator.hxx"

#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>


namespace ice::memory
{

    struct memory_globals
    {
        static constexpr uint32_t Constant_GlobalAllocatorBuffer =
            sizeof(ice::memory::DefaultAllocator)
            + sizeof(ice::memory::ScratchAllocator);

        char _buffer[Constant_GlobalAllocatorBuffer]{ 0 };

        ice::memory::DefaultAllocator* _default_allocator{ nullptr };
        ice::memory::ScratchAllocator* _default_scratch_allocator{ nullptr };

        ice::Allocator* _stats_allocator{ nullptr };
        ice::Allocator* _global_allocator{ nullptr };
    };

    memory_globals g_memory_globals;

    void init(uint32_t temporary_memory) noexcept
    {
        char* ptr = g_memory_globals._buffer;
        g_memory_globals._default_allocator = new (ptr) ice::memory::DefaultAllocator{ };
        g_memory_globals._global_allocator = g_memory_globals._default_allocator;
        ptr += sizeof(ice::memory::DefaultAllocator);
        g_memory_globals._default_scratch_allocator = new (ptr) ice::memory::ScratchAllocator{
            *g_memory_globals._default_allocator,
            temporary_memory
        };

        if constexpr (ice::build::is_debug || ice::build::is_develop)
        {
            g_memory_globals._stats_allocator = g_memory_globals._default_allocator->make<ice::memory::ProxyAllocator>(
                *g_memory_globals._default_allocator,
                "global"
            );

            g_memory_globals._global_allocator = g_memory_globals._stats_allocator;
        }
    }

    void init_with_stats(uint32_t temporary_memory) noexcept
    {
        init(temporary_memory);
    }

    auto default_allocator() noexcept -> ice::Allocator&
    {
        return *g_memory_globals._global_allocator;
    }

    auto default_scratch_allocator() noexcept -> ice::Allocator&
    {
        return *g_memory_globals._default_scratch_allocator;
    }

    void shutdown() noexcept
    {
        g_memory_globals._global_allocator = nullptr;

        if constexpr (ice::build::is_debug || ice::build::is_develop)
        {
            g_memory_globals._stats_allocator->~Allocator();
        }

        g_memory_globals._default_allocator->deallocate(g_memory_globals._stats_allocator);
        g_memory_globals._default_scratch_allocator->~ScratchAllocator();
        g_memory_globals._default_allocator->~DefaultAllocator();
        g_memory_globals = memory_globals{ };
    }

} // namespace ice::memory
