#pragma once
#include <memsys/memsys.h>

namespace mem
{

class MEMSYS_API malloc_allocator : public allocator
{
public:
    malloc_allocator();
    ~malloc_allocator() override;

    void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) override;

    void deallocate(void* ptr) override;

    uint32_t allocated_size(void* ptr) override;

    uint32_t total_allocated() override;

private:
    size_t _allocated;
};

}
