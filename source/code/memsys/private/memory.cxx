#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>
#include "allocators/default_allocator.hxx"


namespace core::memory::globals
{

struct memory_globals
{
    static constexpr uint32_t ALLOCATOR_MEMORY = sizeof(core::memory::default_allocator) + sizeof(core::memory::scratch_allocator);

    char _buffer[ALLOCATOR_MEMORY]{ 0 };

    core::memory::default_allocator* _default_allocator{ nullptr };
    core::memory::scratch_allocator* _default_scratch_allocator{ nullptr };

    core::memory::proxy_allocator* _stats_allocator{ nullptr };
    core::allocator* _global_allocator{ nullptr };
};

memory_globals g_memory_globals;

void init(uint32_t temporary_memory) noexcept
{
    char* ptr = g_memory_globals._buffer;
    g_memory_globals._default_allocator = new (ptr) core::memory::default_allocator{ };
    g_memory_globals._global_allocator = g_memory_globals._default_allocator;
    ptr += sizeof(core::memory::default_allocator);
    g_memory_globals._default_scratch_allocator = new (ptr) core::memory::scratch_allocator{ *g_memory_globals._default_allocator, temporary_memory };
}

void init_with_stats(uint32_t temporary_memory) noexcept
{
    init(temporary_memory);

    // Init the proxy allocator
    g_memory_globals._stats_allocator = g_memory_globals._default_allocator->make<core::memory::proxy_allocator>("global", *g_memory_globals._default_allocator);
    g_memory_globals._global_allocator = g_memory_globals._stats_allocator;
}

auto default_allocator() noexcept -> core::allocator&
{
    return *g_memory_globals._global_allocator;
}

auto default_scratch_allocator() noexcept -> core::allocator&
{
    return *g_memory_globals._default_scratch_allocator;
}

auto null_allocator() noexcept -> core::allocator&
{
    struct null_allocator_type : core::allocator
    {
        auto allocate(uint32_t, uint32_t) noexcept -> void* override { return nullptr; }
        void deallocate(void*) noexcept override { }
        auto allocated_size(void*) noexcept -> uint32_t override { return allocator::SIZE_NOT_TRACKED; }
        auto total_allocated() noexcept -> uint32_t override { return allocator::SIZE_NOT_TRACKED; }
    };

    static null_allocator_type null_allocator;
    return null_allocator;
}

void shutdown() noexcept
{
    g_memory_globals._global_allocator = nullptr;

    g_memory_globals._default_allocator->deallocate(g_memory_globals._stats_allocator);
    g_memory_globals._default_scratch_allocator->~scratch_allocator();
    g_memory_globals._default_allocator->~default_allocator();
    g_memory_globals = memory_globals{ };
}

} // namespace core::memory::globals
