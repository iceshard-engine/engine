/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct ForwardAllocatorParams
    {
        //! \brief The size of a single memory block.
        ice::usize bucket_size = 1_KiB;

        //! \brief The number of empty buckets allocated.
        ice::ucount min_bucket_count = 1;
    };

    struct ForwardAllocator : public ice::Allocator
    {
        ForwardAllocator(
            ice::Allocator& backing_allocator,
            ice::ForwardAllocatorParams params = { },
            std::source_location = std::source_location::current()
        ) noexcept;

        ForwardAllocator(
            ice::Allocator& backing_allocator,
            std::string_view name,
            ice::ForwardAllocatorParams params = { },
            std::source_location = std::source_location::current()
        ) noexcept;

        ~ForwardAllocator() noexcept;

        //! \brief Releases all memory blocks not in-use.
        void reset() noexcept;

    protected:
        struct MemoryBucket;

        auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        void do_deallocate(void* pointer) noexcept override;

    private:
        ice::Allocator& _backing_alloc;
        ice::ForwardAllocatorParams const _params;

        MemoryBucket* _buckets;
    };

} // namespace ice
