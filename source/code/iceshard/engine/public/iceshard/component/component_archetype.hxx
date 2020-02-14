#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard
{

    struct ComponentBlock;

    class ComponentBlockAllocator;

    class ComponentArchetype
    {
    public:
        ComponentArchetype(ComponentBlockAllocator& alloc, core::stringid_arg_type id) noexcept
            : _allocator{ alloc }
            , _identifier{ id }
        {
        }

        ~ComponentArchetype(ComponentBlockAllocator& alloc) noexcept
        {
            while (nullptr != _block)
            {
                auto* next = _block->_next;
                // alloc.destroy(_block);
                // _block = next;
            }
        }

    private:
        ComponentBlockAllocator& _allocator;
        core::stringid_type _identifier;
        ComponentBlock* _block = nullptr;
    };

} // namespace iceshard
