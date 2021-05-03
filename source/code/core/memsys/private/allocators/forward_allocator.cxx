#include <ice/memory/forward_allocator.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <cassert>

namespace ice::memory
{

    //! \brief The representation of a single memory bucket.
    struct ForwardAllocator::MemoryBucket
    {
        //! \brief The next memory bucket to be searched for free memory.
        MemoryBucket* next = nullptr;

        //! \brief The next free location in this bucket.
        void* free = nullptr;

        //! \brief The bucket ending location.
        void* const last = nullptr;
    };

    ForwardAllocator::ForwardAllocator(ice::Allocator& backing, uint32_t bucket_size) noexcept
        : ice::Allocator{ backing }
        , _backing_allocator{ backing }
        , _bucket_list{ nullptr }
        , _bucket_size{ bucket_size }
    {
        _bucket_list = allocate_bucket(_bucket_size);
    }

    ForwardAllocator::~ForwardAllocator() noexcept
    {
        release_all();

        // Release the head bucket.
        _backing_allocator.deallocate(_bucket_list);
    }

    auto ForwardAllocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
    {
        auto* current_head = _bucket_list;

        void* free_location = ptr_align_forward(current_head->free, align);
        auto free_location_remaining_size = ptr_distance(free_location, current_head->last);

        [[unlikely]]
        if (free_location_remaining_size < static_cast<int32_t>(size + align))
        {
            [[unlikely]]
            // The requested size is bigger than the bucket size
            if ((size + align) > _bucket_size)
            {
                auto* new_bucket = allocate_bucket(size + align);

                // We don't update the list head here, so the fist bucket is always a bucket with the default size.
                new_bucket->next = current_head->next;
                current_head->next = new_bucket;

                free_location = ptr_align_forward(new_bucket->free, align);
                new_bucket->free = ptr_add(free_location, size);

                assert(ptr_distance(free_location, new_bucket->last) >= static_cast<int32_t>(size));/*
                    , "Allocated size does't satisfy requested size! [ requested: {}b, allocated: {}b ]"
                    , size, utils::pointer_distance(free_location, new_bucket->last)
                );*/
            }
            else
            {
                auto* new_bucket = allocate_bucket(_bucket_size);

                _bucket_list = new_bucket;
                _bucket_list->next = current_head;
                current_head = _bucket_list;

                free_location = ptr_align_forward(current_head->free, align);
                current_head->free = free_location;

                assert(ptr_distance(free_location, current_head->last) >= static_cast<int32_t>(size));/*
                    , "Allocated size does't satisfy requested size! [ requested: {}b, allocated: {}b ]"
                    , size, utils::pointer_distance(free_location, current_head->last)
                );*/

                current_head->free = ptr_add(free_location, size);
            }
        }
        else
        {
            current_head->free = ptr_add(free_location, size);
        }

        return free_location;
    }

    void ForwardAllocator::deallocate(void*) noexcept
    {
        /* we don't release memory here */
    }

    auto ForwardAllocator::allocated_size(void*) const noexcept -> uint32_t
    {
        return ice::Allocator::Constant_SizeNotTracked;
    }

    auto ForwardAllocator::total_allocated() const noexcept -> uint32_t
    {
        // #todo Implement allocated size counting.
        return ice::Allocator::Constant_SizeNotTracked;
    }

    void ForwardAllocator::release_all() noexcept
    {
        // Release all buckets beside the first one
        // * There is no need to reallocate it
        // * The fist bucket will always have the default bucket size
        auto* it = _bucket_list->next;
        while (it != nullptr)
        {
            auto* next = it->next;
            _backing_allocator.deallocate(it);
            it = next;
        }

        // Reset the 'free' and 'next' pointers
        _bucket_list->free = _bucket_list + 1;
        _bucket_list->next = nullptr;

        auto available_size = ptr_distance(_bucket_list->free, _bucket_list->last);
        assert(available_size == static_cast<int32_t>(_bucket_size));/*
            , "Bucket reset failed, available size does not match default bucket size! [ available: {}b, default: {}b ]"
            , available_size, _bucket_size
        );*/
    }

    auto ForwardAllocator::allocate_bucket(uint32_t size) noexcept -> MemoryBucket*
    {
        const auto required_bucket_size = static_cast<uint32_t>(size + sizeof(MemoryBucket));

        // Allocate enough memory
        void* memory = _backing_allocator.allocate(required_bucket_size, alignof(MemoryBucket));
        void* memory_end = ptr_add(memory, required_bucket_size);

        // Create the new bucket in-place
        auto* new_bucket = new (memory) MemoryBucket{
            .next = nullptr,
            .free = ptr_add(memory, sizeof(MemoryBucket)),
            .last = memory_end
        };

        assert(ptr_distance(new_bucket->free, new_bucket->last) >= static_cast<int32_t>(size));/*
            , "Allocated size does't satisfy requested size! [ requested: {}b, allocated: {}b ]"
            , size, utils::pointer_distance(new_bucket->free, new_bucket->last)
        );*/

        return new_bucket;
    }

} // namespace ice::memory
