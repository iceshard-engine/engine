#pragma once
#include <memsys/allocator.hxx>
#include <string_view>

namespace memsys
{
    class MEMSYS_API proxy_allocator : public allocator
    {
    public:
        proxy_allocator(const char* name, memsys::allocator& alloc) noexcept;
        virtual ~proxy_allocator() noexcept override;

        virtual void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept override;
        virtual void deallocate(void* ptr) noexcept override;

        virtual uint32_t allocated_size(void* ptr) noexcept override;
        virtual uint32_t total_allocated() noexcept override;

        virtual allocator& backing_allocator() noexcept;

    private:
        const char* _name;
        allocator& _allocator;
        uint32_t _total_allocated;
    };
}
