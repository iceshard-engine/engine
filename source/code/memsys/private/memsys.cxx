#include <memsys/memsys.hxx>
#include <memsys/allocators/scratch_allocator.hxx>
#include "allocators/default_allocator.hxx"


namespace memsys::globals
{

struct memory_globals
{
    static constexpr uint32_t ALLOCATOR_MEMORY = sizeof(memsys::default_allocator) + sizeof(scratch_allocator);

    char _buffer[ALLOCATOR_MEMORY]{ 0 };

    memsys::default_allocator* _default_allocator{ nullptr };
    memsys::scratch_allocator* _default_scratch_allocator{ nullptr };
};

memory_globals g_memory_globals;

void init(uint32_t temporary_memory) noexcept
{
    char* ptr = g_memory_globals._buffer;
    g_memory_globals._default_allocator = new (ptr) memsys::default_allocator{ };
    ptr += sizeof(memsys::default_allocator);
    g_memory_globals._default_scratch_allocator = new (ptr) memsys::scratch_allocator{ *g_memory_globals._default_allocator, temporary_memory };
}

auto default_allocator() noexcept -> allocator&
{
    return *g_memory_globals._default_allocator;
}

auto default_scratch_allocator() noexcept -> allocator&
{
    return *g_memory_globals._default_scratch_allocator;
}

void shutdown() noexcept
{
    g_memory_globals._default_scratch_allocator->~scratch_allocator();
    g_memory_globals._default_allocator->~default_allocator();
    g_memory_globals = memory_globals{ };
}

} // namespace memsys::globals
