#pragma once
#include <ice/allocator.hxx>

namespace ice::memory
{

    //! \brief Allocates memory without reusing it later.
    //!
    //! \details This allocator should be use used when allocating simple objects in a single task,
    //!     so the memory can be disposed after the work is done.
    //! \details This allocator uses buckets to allocate more memory once and don't ask the backing allocator every time a object is allocated.
    class ForwardAllocator final : public ice::Allocator
    {
    public:
        ForwardAllocator(ice::Allocator& backing, uint32_t bucket_size) noexcept;
        ~ForwardAllocator() noexcept override;

        auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override;

        void deallocate(void* ptr) noexcept override;

        auto allocated_size(void* ptr) noexcept -> uint32_t override;

        auto total_allocated() noexcept -> uint32_t override;

        void release_all() noexcept;

    protected:
        struct MemoryBucket;

        auto allocate_bucket(uint32_t size) noexcept -> MemoryBucket*;

    private:
        ice::Allocator& _backing_allocator;

        MemoryBucket* _bucket_list;

        uint32_t const _bucket_size;
    };


} // namespace core::memory
