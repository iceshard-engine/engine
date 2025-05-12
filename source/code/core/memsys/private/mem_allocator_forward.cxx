/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_forward.hxx>

namespace ice
{

    struct ForwardAllocator::MemoryBucket
    {
        void* _free;
        void* _beg;
        void* _end;

        // Only used for tracking of deallocations, do not use to calculate pointers!
        ice::ucount _alloc_count;

        MemoryBucket* _next;

        static bool try_dealloc_space(MemoryBucket const& bucket, void* pointer) noexcept;
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
            ICE_ASSERT_CORE(_buckets->_alloc_count == 0);
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
                if (_buckets->_alloc_count == 0)
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
                if (_buckets->_alloc_count == 0)
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

                // Get the pointer location and update the free ptr.
                MemoryBucket::try_alloc_space(*buckets, result);

                // Save the new bucket at the list start
                buckets->_next = ice::exchange(_buckets, buckets);
            }

            // Update the bucket info.
            buckets->_free = ice::ptr_add(result.memory, result.size);

            // We only store the requested size values.
            buckets->_alloc_count += 1;
            return result;
        }
        else // Fall back to the parent allocator
        {
            return _backing_alloc.allocate(request);
        }
    }

    void ForwardAllocator::do_deallocate(void* pointer) noexcept
    {
        MemoryBucket* buckets = _buckets;
        while (buckets != nullptr && MemoryBucket::try_dealloc_space(*buckets, pointer) == false)
        {
            buckets = buckets->_next;
        }

        if (buckets != nullptr)
        {
            buckets->_alloc_count -= 1;
        }
        else
        {
            _backing_alloc.deallocate(pointer);
        }
    }

    bool ForwardAllocator::MemoryBucket::try_dealloc_space(MemoryBucket const& bucket, void* pointer) noexcept
    {
        return bucket._beg <= pointer && bucket._free > pointer;
    }

    bool ForwardAllocator::MemoryBucket::try_alloc_space(MemoryBucket const& bucket, ice::AllocResult& result) noexcept
    {
        result.memory = ice::align_to(bucket._free, result.alignment).value;
        return ice::ptr_distance(result.memory, bucket._end) >= result.size;
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
        new_bucket->_alloc_count = 0;
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
