#pragma once
#include <memsys/allocator.h>

namespace mem
{
    class MEMSYS_API forward_allocator : public allocator
    {
    public:
        forward_allocator(mem::allocator& backing, unsigned bucket_size);
        virtual ~forward_allocator() override;

        virtual void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) override;
        virtual void deallocate(void* ptr) override;
        virtual uint32_t allocated_size(void* ptr) override;
        virtual uint32_t total_allocated() override;

        void release_all();

    private:
        struct memory_bucket
        {
            memory_bucket* next = nullptr;

            void* free = nullptr;
            void* last = nullptr;
        };

        mem::allocator& _backing;
        memory_bucket* _buckets;
        const unsigned _bucket_size;
    };
}
