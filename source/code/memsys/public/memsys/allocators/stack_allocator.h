#pragma once
#include <memsys/memsys.h>
#include <cassert>

namespace memsys
{
    template<uint32_t BUFFER_SIZE>
    class stack_allocator : public allocator
    {
    public:
        stack_allocator() : _buffer{ }, _next{ _buffer } { }
        virtual ~stack_allocator() override
        {
            auto size = total_allocated();
            assert(size <= BUFFER_SIZE);
        }

        virtual void* allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) override
        {
            assert(size <= sizeof(_buffer));
            void* ptr = utils::align_forward(_next, align);
            void* next = utils::pointer_add(ptr, size);
            assert(next <= (_buffer + size));
            _next = reinterpret_cast<char*>(next);
            return ptr;
        }

        virtual void deallocate(void* /*ptr*/) override
        {
            /* Unused */
        }

        virtual uint32_t allocated_size(void* /*ptr*/) override
        {
            return SIZE_NOT_TRACKED;
        }

        virtual uint32_t total_allocated() override
        {
            return static_cast<uint32_t>(_next - _buffer);
        }

        void clear()
        {
            _next = _buffer;
        }

    private:
        char _buffer[BUFFER_SIZE];
        char* _next;
    };

    using stack_allocator_512 = stack_allocator<512>;
    using stack_allocator_1024 = stack_allocator<1024>;
    using stack_allocator_2048 = stack_allocator<2048>;
    using stack_allocator_4096 = stack_allocator<4096>;
}
