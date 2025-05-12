/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    struct RingAllocatorParams
    {
        //! \brief The size of the ring buffer.
        ice::usize ring_buffer_size = 4_KiB;
    };

    struct RingAllocator : public ice::Allocator
    {
        RingAllocator(
            ice::Allocator& backing_allocator,
            ice::RingAllocatorParams params = { },
            std::source_location = std::source_location::current()
        ) noexcept;

        RingAllocator(
            ice::Allocator& backing_allocator,
            std::string_view name,
            ice::RingAllocatorParams params = { },
            std::source_location = std::source_location::current()
        ) noexcept;

        ~RingAllocator() noexcept;

        //! \brief Releases all memory blocks not in-use.
        void reset() noexcept;

    protected:
        struct MemoryBucket;

        auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        void do_deallocate(void* pointer) noexcept override;

        bool is_locked(void* ptr) const noexcept;
        bool is_backed(void* ptr) const noexcept;

    private:
        ice::Allocator& _backing_alloc;
        ice::RingAllocatorParams const _params;

        void* _begin;
        void* _end;

        void* _allocate;
        void* _free;
    };

} // namespace ice
