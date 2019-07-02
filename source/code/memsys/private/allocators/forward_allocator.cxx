#include <core/allocators/forward_allocator.hxx>
#include <core/memory.hxx>
#include <core/debug/assert.hxx>

namespace core::memory
{

    //! \brief The representation of a single memory bucket.
    struct forward_allocator::memory_bucket
    {
        //! \brief The next memory bucket to be searched for free memory.
        memory_bucket* next = nullptr;

        //! \brief The next free location in this bucket.
        void* free = nullptr;

        //! \brief The bucket ending location.
        void* const last = nullptr;
    };

    forward_allocator::forward_allocator(core::allocator& backing, unsigned bucket_size) noexcept
        : _backing_allocator{ backing }
        , _bucket_list{ nullptr }
        , _bucket_size{ bucket_size }
    {
        _bucket_list = allocate_bucket(_bucket_size);
    }

    forward_allocator::~forward_allocator() noexcept
    {
        release_all();

        // Release the head bucket.
        _backing_allocator.deallocate(_bucket_list);
    }

    auto forward_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/) noexcept -> void*
    {
        auto* current_head = _bucket_list;

        void* free_location = utils::align_forward(current_head->free, align);
        auto free_location_remaining_size = utils::pointer_distance(free_location, current_head->last);

        [[unlikely]]
        if (free_location_remaining_size < static_cast<int32_t>(size + align))
        {
            [[unlikely]]
            // The requested size is bigger than the bucket size
            if (size + align > _bucket_size)
            {
                auto* new_bucket = allocate_bucket(size + align);

                // We don't update the list head here, so the fist bucket is always a bucket with the default size.
                new_bucket->next = current_head->next;
                current_head->next = new_bucket;

                free_location = utils::align_forward(new_bucket->free, align);
                new_bucket->free = utils::pointer_add(free_location, size);

                IS_ASSERT(utils::pointer_distance(free_location, new_bucket->last) == static_cast<int32_t>(size)
                    , "Allocated size does't satisfy requested size! [ requested: {}b, allocated: {}b ]"
                    , size, utils::pointer_distance(free_location, new_bucket->last)
                );
            }
            else
            {
                auto* new_bucket = allocate_bucket(_bucket_size);

                _bucket_list = new_bucket;
                _bucket_list->next = current_head;
                current_head = _bucket_list;

                free_location = utils::align_forward(current_head->free, align);
                current_head->free = free_location;

                IS_ASSERT(utils::pointer_distance(free_location, current_head->last) >= static_cast<int32_t>(size)
                    , "Allocated size does't satisfy requested size! [ requested: {}b, allocated: {}b ]"
                    , size, utils::pointer_distance(free_location, current_head->last)
                );
            }
        }

        current_head->free = utils::pointer_add(free_location, size);
        IS_ASSERT(utils::pointer_distance(current_head->free, current_head->last) >= 0
            , "Allocation failed, the free pointer moved past the end! [ requested size: {}b, requested align: {}b ]"
            , size, align
        );

        // Return the free pointer
        return free_location;
    }

    void forward_allocator::deallocate(void*) noexcept
    {
        /* we don't release memory here */
    }

    auto forward_allocator::allocated_size(void*) noexcept -> uint32_t
    {
        return SIZE_NOT_TRACKED;
    }

    auto forward_allocator::total_allocated() noexcept -> uint32_t
    {
        // #todo Implement allocated size counting.
        return SIZE_NOT_TRACKED;
    }

    void forward_allocator::release_all() noexcept
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

        auto available_size = utils::pointer_distance(_bucket_list->free, _bucket_list->last);
        IS_ASSERT(available_size == static_cast<int32_t>(_bucket_size)
            , "Bucket reset failed, available size does not match default bucket size! [ available: {}b, default: {}b ]"
            , available_size, _bucket_size
        );
    }

    auto forward_allocator::allocate_bucket(uint32_t size) noexcept -> memory_bucket*
    {
        const auto required_bucket_size = static_cast<uint32_t>(size + sizeof(memory_bucket));

        // Allocate enough memory
        void* memory = _backing_allocator.allocate(required_bucket_size, alignof(memory_bucket));
        void* memory_end = utils::pointer_add(memory, required_bucket_size);

        // Create the new bucket in-place
        auto* new_bucket = new (memory) memory_bucket{
            /* set the current head bucket as the tail */
            nullptr
            /* set to end */
            , memory_end
            /* set the free location already accommodating for the requested objects alignment */
            , utils::pointer_add(memory, sizeof(memory_bucket))
        };

        IS_ASSERT(utils::pointer_distance(new_bucket->free, new_bucket->last) >= static_cast<int32_t>(size)
            , "Allocated size does't satisfy requested size! [ requested: {}b, allocated: {}b ]"
            , size, utils::pointer_distance(new_bucket->free, new_bucket->last)
        );

        return new_bucket;
    }

} // namespace core::memory
