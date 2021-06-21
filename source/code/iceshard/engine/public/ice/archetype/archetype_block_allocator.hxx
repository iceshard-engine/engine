#pragma once
#include <ice/allocator.hxx>
#include <ice/archetype/archetype_block.hxx>
#include <atomic>

namespace ice
{

    class ArchetypeBlockAllocator
    {
    public:
        ArchetypeBlockAllocator(
            ice::Allocator& alloc
        ) noexcept;
        ~ArchetypeBlockAllocator() noexcept;

        auto block_size() const -> ice::u32;

        auto allocate_block() noexcept -> ice::ArchetypeBlock*;
        void release_block(ice::ArchetypeBlock* block) noexcept;

    private:
        ice::Allocator& _allocator;
        std::atomic<ice::ArchetypeBlock*> _block_list;
    };

} // namespace ice
