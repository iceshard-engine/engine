#include <core/allocators/scratch_allocator.hxx>
#include <core/memory.hxx>
#include "allocator_utils.hxx"

#include <core/debug/assert.hxx>

namespace core::memory
{
    namespace detail
    {

        static constexpr auto FREE_MEMORY_BIT = 0x80000000u;

        static constexpr auto FREE_MEMORY_MASK = ~FREE_MEMORY_BIT;

        //! \brief Returns the header object from the given raw pointer.
        auto get_header(void* pointer) noexcept -> tracking::allocation_header*
        {
            return reinterpret_cast<tracking::allocation_header*>(
                utils::align_forward(pointer, alignof(tracking::allocation_header))
                );
        }

    } // namespace detail

    scratch_allocator::scratch_allocator(allocator& backing, uint32_t size) noexcept
        : _backing{ backing }
    {
        _begin = _backing.allocate(size, alignof(tracking::allocation_header));
        _end = utils::pointer_add(_begin, size);

        _allocate = _begin;
        _free = _begin;

        std::memset(_begin, 0, utils::pointer_distance(_begin, _end));
    }

    scratch_allocator::~scratch_allocator() noexcept
    {
        IS_ASSERT(_free == _allocate, "Unreleased memory in scratch allocator!");
        _backing.deallocate(_begin);
    }

    auto scratch_allocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
    {
        IS_ASSERT((align % 4) == 0, "Invalid alignment value '{}' passed to allocation function!", align);

        void* candidate_pointer = _allocate;

        auto* alloc_header = detail::get_header(candidate_pointer);
        auto* alloc_data = tracking::data_pointer(alloc_header, align);
        void* alloc_data_end = utils::pointer_add(alloc_data, size);

        [[unlikely]]
        if (alloc_data_end > _end)
        {
            // Save the amount of bytes we are ignoring.
            alloc_header->allocated_size = utils::pointer_distance(alloc_header, _end) | detail::FREE_MEMORY_BIT;

            // The new candidate for the allocation
            candidate_pointer = _begin;

            alloc_header = detail::get_header(candidate_pointer);
            alloc_data = tracking::data_pointer(alloc_header, align);
            alloc_data_end = utils::pointer_add(alloc_data, size);
        }

        [[unlikely]]
        if (is_locked(alloc_data_end))
        {
            return _backing.allocate(size, align);
        }

        _allocate = alloc_data_end;
        tracking::fill(alloc_header, alloc_data, utils::pointer_distance(alloc_header, alloc_data_end), size);
        return alloc_data;
    }

    void scratch_allocator::deallocate(void* pointer) noexcept
    {
        if (nullptr == pointer)
        {
            return;
        }

        if (is_backing_pointer(pointer))
        {
            _backing.deallocate(pointer);
            return;
        }

        // Get the associated allocation header
        auto* alloc_header = tracking::header(pointer);

        IS_ASSERT((alloc_header->allocated_size & detail::FREE_MEMORY_BIT) == 0, "The allocation header is already freed!");
        alloc_header->allocated_size = alloc_header->allocated_size | detail::FREE_MEMORY_BIT;

        // We don't need 'h' anymore.
        alloc_header = nullptr;

        // Advance the free pointer past all free slots.
        while (_free != _allocate)
        {
            alloc_header = detail::get_header(_free);

            // Until we find an locked memory segment.
            if ((alloc_header->allocated_size & detail::FREE_MEMORY_BIT) == 0)
            {
                break;
            }

            // Move the free pointer by the given amount of bytes.
            _free = utils::pointer_add(alloc_header, alloc_header->allocated_size & detail::FREE_MEMORY_MASK);
            if (_free == _end)
            {
                _free = _begin;
            }
        }
    }

    auto scratch_allocator::allocated_size(void* pointer) noexcept -> uint32_t
    {
        if (is_backing_pointer(pointer))
        {
            return _backing.allocated_size(pointer);
        }
        else
        {
            auto alloc_header = tracking::header(pointer);
            return alloc_header->requested_size;
        }
    }

    auto scratch_allocator::total_allocated() noexcept -> uint32_t
    {
        auto distance = utils::pointer_distance(_free, _allocate);
        if (distance < 0)
        {
            distance += utils::pointer_distance(_begin, _end);
        }
        return distance;
    }

    bool scratch_allocator::reset() noexcept
    {
        const bool empty = total_allocated() == 0;

        // Always reset pointers on the allocator!
        _allocate = _begin;
        _free = _begin;

        // Set the memory to zeros
        std::memset(_begin, 0, utils::pointer_distance(_begin, _end));
        return empty;
    }

    bool scratch_allocator::is_locked(void* pointer) noexcept
    {
        [[unlikely]]
        if (_free == _allocate)
        {
            return false;
        }

        if (_allocate > _free)
        {
            return pointer >= _free && pointer < _allocate;
        }
        return pointer >= _free || pointer < _allocate;
    }

    bool scratch_allocator::is_backing_pointer(void* pointer) noexcept
    {
        return pointer < _begin || pointer >= _end;
    }


} // namespace core::memory
