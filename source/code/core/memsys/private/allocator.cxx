#include <ice/allocator.hxx>

namespace ice::memory
{

    auto null_allocator() noexcept -> Allocator&
    {
        struct NullAllocator : Allocator
        {
            auto allocate(uint32_t, uint32_t) noexcept -> void* override { return nullptr; }
            void deallocate(void*) noexcept override { }
            auto allocated_size(void*) noexcept -> uint32_t override { return 0; }
            auto total_allocated() noexcept -> uint32_t override { return 0; }
        };

        static NullAllocator null_allocator_instance;
        return null_allocator_instance;
    }

} // namespace ice
