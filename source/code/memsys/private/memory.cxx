#include <core/memory.hxx>
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
};

memory_globals g_memory_globals;

void init(uint32_t temporary_memory) noexcept
{
    char* ptr = g_memory_globals._buffer;
    g_memory_globals._default_allocator = new (ptr) core::memory::default_allocator{ };
    ptr += sizeof(core::memory::default_allocator);
    g_memory_globals._default_scratch_allocator = new (ptr) core::memory::scratch_allocator{ *g_memory_globals._default_allocator, temporary_memory };
}

auto default_allocator() noexcept -> core::allocator&
{
    return *g_memory_globals._default_allocator;
}

auto default_scratch_allocator() noexcept -> core::allocator&
{
    return *g_memory_globals._default_scratch_allocator;
}

void shutdown() noexcept
{
    g_memory_globals._default_scratch_allocator->~scratch_allocator();
    g_memory_globals._default_allocator->~default_allocator();
    g_memory_globals = memory_globals{ };
}

} // namespace core::memory::globals
