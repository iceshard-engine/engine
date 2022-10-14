#include <ice/mem_allocator_forward.hxx>

namespace ice
{

    struct ForwardAllocator::MemoryBucket
    {
        void* _free;
        void* _beg;
        void* _end;

        // Only used for tracking of deallocations, do not use to calculate pointers!
        ice::usize _inuse;

        MemoryBucket* _next;

        static bool try_dealloc_space(MemoryBucket const& bucket, ice::Memory memory) noexcept;
        static bool try_alloc_space(MemoryBucket const& bucket, ice::AllocResult& result) noexcept;

        static auto alloc_bucket(
            ice::Allocator& alloc,
            ice::ForwardAllocatorParams const& params
        ) noexcept -> MemoryBucket*;

        static auto alloc_buckets(
            ice::Allocator& alloc,
            ice::ForwardAllocatorParams const& params,
            ice::ucount count
        ) noexcept -> MemoryBucket*;
    };


    ForwardAllocator::ForwardAllocator(
        ice::Allocator& backing_allocator,
        ice::ForwardAllocatorParams params,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator }
        , _backing_alloc{ backing_allocator }
        , _params{ params }
        , _buckets{ MemoryBucket::alloc_buckets(_backing_alloc, _params, _params.min_bucket_count) }
    {
    }

    ForwardAllocator::ForwardAllocator(
        ice::Allocator& backing_allocator,
        std::string_view name,
        ice::ForwardAllocatorParams params,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator, name }
        , _backing_alloc{ backing_allocator }
        , _params{ params }
        , _buckets{ MemoryBucket::alloc_buckets(_backing_alloc, _params, _params.min_bucket_count) }
    {
    }

    ForwardAllocator::~ForwardAllocator() noexcept
    {
        while (_buckets != nullptr)
        {
            ICE_ASSERT_CORE(_buckets->_inuse == 0_B);
            MemoryBucket* next = _buckets->_next;

            _backing_alloc.deallocate(
                Memory{
                    .location = _buckets,
                    .size = _params.bucket_size,
                    .alignment = ice::align_of<MemoryBucket>
                }
            );

            _buckets = next;
        }
    }

    void ForwardAllocator::reset() noexcept
    {
        ice::ucount skipped_bucket_count = 0;

        MemoryBucket* new_list = nullptr;
        while (_buckets != nullptr)
        {
            MemoryBucket* next = _buckets->_next;
            if (skipped_bucket_count == _params.min_bucket_count)
            {
                if (_buckets->_inuse == 0_B)
                {
                    _backing_alloc.deallocate(
                        Memory{
                            .location = _buckets,
                            .size = _params.bucket_size,
                            .alignment = ice::align_of<MemoryBucket>
                        }
                    );
                }
                else
                {
                    _buckets->_next = ice::exchange(new_list, _buckets);
                }
            }
            else
            {
                // Reset the free pointer in empty blocks.
                if (_buckets->_inuse == 0_B)
                {
                    _buckets->_free = _buckets->_beg;
                }

                _buckets->_next = ice::exchange(new_list, _buckets);
                skipped_bucket_count += 1;
            }

            _buckets = next;
        }

        // Store the new list
        _buckets = new_list;
    }

    auto ForwardAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        ice::AllocResult result{ .size = request.size, .alignment = request.alignment };

        // Only lookf for buckets that have enough size.
        if (request.size < (_params.bucket_size - ice::size_of<MemoryBucket>))
        {
            MemoryBucket* buckets = _buckets;
            while (buckets != nullptr && MemoryBucket::try_alloc_space(*buckets, result) == false)
            {
                buckets = buckets->_next;
            }

            if (buckets == nullptr)
            {
                buckets = MemoryBucket::alloc_bucket(_backing_alloc, _params);
                ICE_ASSERT_CORE(ice::ptr_distance(buckets->_beg, buckets->_end) >= request.size);

                // Get the memory location and update the free ptr.
                MemoryBucket::try_alloc_space(*buckets, result);

                // Save the new bucket at the list start
                buckets->_next = ice::exchange(_buckets, buckets);
            }

            // Update the bucket info.
            buckets->_free = ice::ptr_add(buckets->_free, result.size);

            // We only store the requested size values.
            buckets->_inuse += result.size;
            return result;
        }
        else // Fall back to the parent allocator
        {
            return _backing_alloc.allocate(request);
        }
    }

    void ForwardAllocator::do_deallocate(ice::Memory memory) noexcept
    {
        // Check if this allocator could be responsible for this memory.
        if (memory.size < (_params.bucket_size - ice::size_of<MemoryBucket>))
        {
            MemoryBucket* buckets = _buckets;
            while (buckets != nullptr && MemoryBucket::try_dealloc_space(*buckets, memory) == false)
            {
                buckets = buckets->_next;
            }

            ICE_ASSERT_CORE(buckets != nullptr);

            // We only store the requested size values.
            buckets->_inuse = { buckets->_inuse.value - memory.size.value };
        }
        else
        {
            _backing_alloc.deallocate(memory);
        }
    }

    bool ForwardAllocator::MemoryBucket::try_dealloc_space(MemoryBucket const& bucket, ice::Memory memory) noexcept
    {
        return bucket._beg <= memory.location && bucket._free > memory.location;
    }

    bool ForwardAllocator::MemoryBucket::try_alloc_space(MemoryBucket const& bucket, ice::AllocResult& result) noexcept
    {
        result.memory = ice::align_to(bucket._free, result.alignment).value;
        return ice::ptr_distance(result, bucket._end) >= result.size;
    }

    auto ForwardAllocator::MemoryBucket::alloc_bucket(
        ice::Allocator& alloc,
        ice::ForwardAllocatorParams const& params
    ) noexcept -> MemoryBucket*
    {
        // No bucket found, so we allocate a new one.
        ice::AllocResult const alloc_result = alloc.allocate({ params.bucket_size, ice::align_of<MemoryBucket> });

        MemoryBucket* new_bucket = reinterpret_cast<MemoryBucket*>(alloc_result.memory);
        new_bucket->_beg = new_bucket + 1;
        new_bucket->_end = ice::ptr_add(new_bucket, alloc_result.size);
        new_bucket->_free = new_bucket->_beg;
        new_bucket->_inuse = 0_B;
        new_bucket->_next = nullptr;
        return new_bucket;
    }

    auto ForwardAllocator::MemoryBucket::alloc_buckets(
        ice::Allocator& alloc,
        ice::ForwardAllocatorParams const& params,
        ice::ucount count
    ) noexcept -> MemoryBucket*
    {
        MemoryBucket* result = nullptr;
        while (count > 0)
        {
            MemoryBucket* new_bucket = alloc_bucket(alloc, params);
            new_bucket->_next = result;
            result = new_bucket;

            count -= 1;
        }
        return result;
    }

} // namespace ice
