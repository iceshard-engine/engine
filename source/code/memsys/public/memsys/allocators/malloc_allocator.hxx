#pragma once
#include <memsys/memsys.hxx>

namespace memsys
{

class MEMSYS_API malloc_allocator : public allocator
{
public:
    malloc_allocator() noexcept;
    ~malloc_allocator() noexcept override;

    void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept override;

    void deallocate(void* ptr) noexcept override;

    uint32_t allocated_size(void* ptr) noexcept override;

    uint32_t total_allocated() noexcept override;

private:
    size_t _allocated;
};

}
