#pragma once
#include <ice/mem_allocator.hxx>
#include <arctic/arctic_syntax_node_allocator.hxx>

namespace ice
{

    class ASLAllocator : public arctic::SyntaxNodeAllocator
    {
    public:
        inline ASLAllocator(ice::Allocator& alloc) noexcept
            : _backing{ alloc }
        { }

        inline ASLAllocator(ice::ASLAllocator& alloc) noexcept
            : _backing{ alloc._backing }
        { }

        inline auto allocate(arctic::usize size, arctic::usize align) noexcept -> void* override
        {
            return _backing.allocate({ ice::usize{ size }, static_cast<ice::ualign>(align) }).memory;
        }

        inline void deallocate(void* ptr) noexcept override
        {
            _backing.deallocate(ptr);
        }

    public:
        ice::Allocator& _backing;
    };

} // namespace ice
