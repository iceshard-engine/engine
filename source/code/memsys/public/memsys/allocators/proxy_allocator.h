#pragma once
#include <memsys/allocator.h>
#include <string_view>

namespace memsys
{
    class MEMSYS_API proxy_allocator : public allocator
    {
    public:
        proxy_allocator(const char* name, memsys::allocator& alloc);
        virtual ~proxy_allocator() override;

        virtual void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) override;
        virtual void deallocate(void* ptr) override;

        virtual uint32_t allocated_size(void* ptr) override;
        virtual uint32_t total_allocated() override;

        virtual allocator& backing_allocator();

    private:
        const char* _name;
        allocator& _allocator;
        uint32_t _total_allocated;
    };
}
