#include <memsys/memsys.h>
#include <memsys/allocators/forward_allocator.h>
#include <cassert>


namespace memsys
{

forward_allocator::forward_allocator(allocator& backing, unsigned bucket_size) noexcept
    : _backing{ backing }
    , _buckets{ nullptr }
    , _bucket_size{ bucket_size }
{
    release_all();
}

forward_allocator::~forward_allocator() noexcept
{
    release_all();
    _backing.deallocate(_buckets); // Release the root bucket too
}

void* forward_allocator::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/) noexcept
{
    memory_bucket* bucket = _buckets;
    void* free_ptr = utils::align_forward(bucket->free, align);
    auto free_size = static_cast<uint32_t>(utils::pointer_distance(free_ptr, bucket->last));

    // Well played
    if (free_size < size)
    {
        // Create a special bucket
        if (size + align > _bucket_size)
        {
            // Create a bucket which size is enough to store the requested size
            void* memory = _backing.allocate(size + align + sizeof(memory_bucket));
            auto* new_bucket = reinterpret_cast<memory_bucket*>(memory);
            new_bucket->next = bucket->next;
            new_bucket->last = utils::pointer_add(new_bucket + 1, size + align);
            new_bucket->free = new_bucket->last;
            bucket->next = new_bucket;

            // Update the bucket and free_ptr locals
            bucket = new_bucket;
            free_ptr = utils::align_forward(new_bucket + 1, align);
            assert(static_cast<uint32_t>(utils::pointer_distance(free_ptr, new_bucket->last)) >= size);
        }
        else // Just create a new bucket
        {
            void* memory = _backing.allocate(_bucket_size + sizeof(memory_bucket));
            auto* new_bucket = reinterpret_cast<memory_bucket*>(memory);
            new_bucket->next = bucket;
            new_bucket->free = new_bucket + 1;
            new_bucket->last = utils::pointer_add(new_bucket->free, _bucket_size);
            _buckets = new_bucket;

            // Update the bucket and free_ptr locals
            bucket = new_bucket;
            free_ptr = utils::align_forward(new_bucket->free, align);
            assert(static_cast<uint32_t>(utils::pointer_distance(free_ptr, new_bucket->last)) >= size);
        }
    }

    // Move forward the free pointer
    bucket->free = utils::pointer_add(free_ptr, size);
    assert(utils::pointer_distance(bucket->free, bucket->last) >= 0);

    // Return the free pointer
    return free_ptr;
}

void forward_allocator::deallocate(void* /*ptr*/) noexcept
{
    /* We don't deallocate anything here */
}

uint32_t forward_allocator::allocated_size(void* /*ptr*/) noexcept
{
    return SIZE_NOT_TRACKED;
}

uint32_t forward_allocator::total_allocated() noexcept
{
    return SIZE_NOT_TRACKED;
}

void forward_allocator::release_all() noexcept
{
    // Try to release everything
    memory_bucket* next = nullptr;
    while (nullptr != _buckets)
    {
        next = _buckets->next;
        _backing.deallocate(_buckets);
        _buckets = next;
    }

    // Create a new root bucket
    void* memory = _backing.allocate(_bucket_size + sizeof(memory_bucket));
    auto* bucket = reinterpret_cast<memory_bucket*>(memory);
    bucket->next = nullptr;
    bucket->free = bucket + 1;
    bucket->last = utils::pointer_add(bucket->free, _bucket_size);
    _buckets = bucket;

    // Check the first bucket
    assert(static_cast<uint32_t>(utils::pointer_distance(_buckets->free, _buckets->last)) == _bucket_size);
}


} // namespace memsys
