#include "allocator_utils.hxx"

#include <ice/memory/scratch_allocator.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
#include <cassert>

//#include <core/debug/assert.hxx>

namespace ice::memory
{
    namespace detail
    {

        static constexpr auto Constant_FreeMemoryBit = 0x80000000u;

        static constexpr auto Constant_FreeMemoryMask = ~Constant_FreeMemoryBit;

        //! \brief Returns the header object from the given raw pointer.
        auto get_header(void* pointer) noexcept -> tracking::AllocationHeader*
        {
            return reinterpret_cast<tracking::AllocationHeader*>(
                ptr_align_forward(pointer, alignof(tracking::AllocationHeader))
            );
        }

    } // namespace detail

    ScratchAllocator::ScratchAllocator(ice::Allocator& backing, uint32_t size, std::string_view name) noexcept
        : ice::Allocator{ backing, name }
        , _backing{ backing }
    {
        _begin = _backing.allocate(size, alignof(tracking::AllocationHeader));
        _end = ptr_add(_begin, size);

        _allocate = _begin;
        _free = _begin;
    }

    ScratchAllocator::~ScratchAllocator() noexcept
    {
        assert(_free == _allocate); // , "Unreleased memory in scratch allocator!");
        _backing.deallocate(_begin);
    }

    auto ScratchAllocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
    {
        assert((align % 4) == 0); // , "Invalid alignment value '{}' passed to allocation function!", align);

        void* candidate_pointer = _allocate;

        auto* alloc_header = detail::get_header(candidate_pointer);
        auto* alloc_data = tracking::data_pointer(alloc_header, align);
        void* alloc_data_end = ptr_add(alloc_data, size);

        [[unlikely]]
        if (alloc_data_end >= _end)
        {
            // Save the amount of bytes we are ignoring.
            alloc_header->allocated_size = ptr_distance(alloc_header, _end) | detail::Constant_FreeMemoryBit;

            // The new candidate for the allocation
            candidate_pointer = _begin;

            alloc_header = detail::get_header(candidate_pointer);
            alloc_data = tracking::data_pointer(alloc_header, align);
            alloc_data_end = ptr_add(alloc_data, size);
        }

        [[unlikely]]
        if (is_locked(alloc_data_end))
        {
            return _backing.allocate(size, align);
        }

        _allocate = alloc_data_end;
        tracking::fill(alloc_header, alloc_data, ptr_distance(alloc_header, alloc_data_end), size);
        return alloc_data;
    }

    void ScratchAllocator::deallocate(void* pointer) noexcept
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

        assert((alloc_header->allocated_size & detail::Constant_FreeMemoryBit) == 0);// , "The allocation header is already freed!");
        alloc_header->allocated_size = alloc_header->allocated_size | detail::Constant_FreeMemoryBit;

        // We don't need 'h' anymore.
        alloc_header = nullptr;

        // Advance the free pointer past all free slots.
        while (_free != _allocate)
        {
            alloc_header = detail::get_header(_free);

            // Until we find an locked memory segment.
            if ((alloc_header->allocated_size & detail::Constant_FreeMemoryBit) == 0)
            {
                break;
            }

            // Move the free pointer by the given amount of bytes.
            _free = ptr_add(alloc_header, alloc_header->allocated_size & detail::Constant_FreeMemoryMask);
            if (_free == _end)
            {
                _free = _begin;
            }
        }
    }

    auto ScratchAllocator::allocated_size(void* pointer) const noexcept -> uint32_t
    {
        if (is_backing_pointer(pointer))
        {
            return _backing.allocated_size(pointer);
        }
        else
        {
            tracking::AllocationHeader const* alloc_header = tracking::header(pointer);
            if ((alloc_header->allocated_size & detail::Constant_FreeMemoryBit) == 0)
            {
                return alloc_header->requested_size;
            }
            else
            {
                return Constant_SizeNotTracked;
            }
        }
    }

    auto ScratchAllocator::total_allocated() const noexcept -> uint32_t
    {
        auto distance = ptr_distance(_free, _allocate);
        if (distance < 0)
        {
            distance += ptr_distance(_begin, _end);
        }
        return distance;
    }

    bool ScratchAllocator::reset_and_discard() noexcept
    {
        const bool discarded_memory = total_allocated() > 0;

        // Always reset pointers on the allocator!
        _allocate = _begin;
        _free = _begin;

        // Set the memory to zeros
        return discarded_memory;
    }

    bool ScratchAllocator::is_locked(void* pointer) noexcept
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

    bool ScratchAllocator::is_backing_pointer(void* pointer) const noexcept
    {
        return pointer < _begin || pointer >= _end;
    }

} // namespace ice::memory
