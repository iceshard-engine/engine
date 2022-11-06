/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_ring.hxx>
#include <ice/assert_core.hxx>

namespace ice::mem
{

    struct AllocationHeader
    {
        ice::usize requested_size;
        ice::usize allocated_size;
    };

    static constexpr ice::u32 Constant_HeaderPadValue = { 0xffff'ffffu };

    inline void* data_pointer(AllocationHeader* const header, ice::ualign align) noexcept
    {
        return ice::align_to(header + 1, align).value;
    }

    inline auto header(void* const data) noexcept -> AllocationHeader*
    {
        auto* temp_pointer = reinterpret_cast<ice::u32*>(data);

        // Subtract the pointer for every HEADER_PAD_VALUE we encounter.
        while (temp_pointer[-1] == Constant_HeaderPadValue)
        {
            --temp_pointer;
        }

        // Return the pointer subtracted by the size of the allocation header.
        return reinterpret_cast<AllocationHeader*>(
            ice::ptr_sub(temp_pointer, ice::size_of<AllocationHeader>)
        );
    }

    // \brief Stores the size in the header and pads with HEADER_PAD_VALUE up to the data pointer.
    inline void fill(
        AllocationHeader* header,
        void const* data_pointer,
        ice::usize allocated_size,
        ice::usize requested_size
    ) noexcept
    {
        header->allocated_size = allocated_size;
        header->requested_size = requested_size;

        auto* header_pointer = reinterpret_cast<ice::u32*>(header + 1);
        while (header_pointer < data_pointer)
        {
            *header_pointer = Constant_HeaderPadValue;
            header_pointer += 1;
        }
    }

} // namespace ice::mem

namespace ice
{

    namespace detail
    {

        static constexpr auto Constant_FreeMemoryBit = 0x80000000u;

        static constexpr auto Constant_FreeMemoryMask = ~Constant_FreeMemoryBit;

        //! \brief Returns the header object from the given raw pointer.
        auto get_header(void* pointer) noexcept -> mem::AllocationHeader*
        {
            return reinterpret_cast<mem::AllocationHeader*>(
                ice::align_to(pointer, ice::align_of<mem::AllocationHeader>).value
            );
        }

    } // namespace detail


    RingAllocator::RingAllocator(
        ice::Allocator& backing_allocator,
        ice::RingAllocatorParams params,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator }
        , _backing_alloc{ backing_allocator }
        , _params{ params }
        , _begin{ nullptr }
        , _end{ nullptr }
        , _allocate{ nullptr }
        , _free{ nullptr }
    {
        _begin = _backing_alloc.allocate({ params.ring_buffer_size, ice::ualign::b_default }).memory;
        _end = ptr_add(_begin, params.ring_buffer_size);

        _allocate = _begin;
        _free = _begin;
    }

    RingAllocator::RingAllocator(
        ice::Allocator& backing_allocator,
        std::string_view name,
        ice::RingAllocatorParams params,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator, name }
        , _backing_alloc{ backing_allocator }
        , _params{ params }
        , _begin{ nullptr }
        , _end{ nullptr }
        , _allocate{ nullptr }
        , _free{ nullptr }
    {
        _begin = _backing_alloc.allocate({ params.ring_buffer_size, ice::ualign::b_default }).memory;
        _end = ptr_add(_begin, params.ring_buffer_size);

        _allocate = _begin;
        _free = _begin;
    }

    RingAllocator::~RingAllocator() noexcept
    {
        ICE_ASSERT_CORE(_free == _allocate);
        _backing_alloc.deallocate({ .location = _begin, .size = _params.ring_buffer_size, .alignment = ice::ualign::b_default });
    }

    auto RingAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        request.alignment = ice::max(request.alignment, request.alignment);
        if (request.size + ice::size_of<mem::AllocationHeader> > _params.ring_buffer_size)
        {
            return _backing_alloc.allocate(request);
        }

        void* candidate_pointer = _allocate;
        auto* alloc_header = detail::get_header(candidate_pointer);
        auto* alloc_data = mem::data_pointer(alloc_header, request.alignment);
        void* alloc_data_end = ice::ptr_add(alloc_data, request.size);

        // If we don't have enough space to store the data requested.
        if (alloc_data_end >= _end)
        {
            // First we need to check if we even can write into the header!
            if (ice::ptr_offset(alloc_header, _end) >= ice::size_of<mem::AllocationHeader>)
            {
                // Save the amount of bytes we are ignoring.
                alloc_header->allocated_size = {
                    ice::ptr_distance(alloc_header, _end).value | detail::Constant_FreeMemoryBit
                };
            }

            // The new candidate for the allocation
            candidate_pointer = _begin;

            alloc_header = detail::get_header(candidate_pointer);
            alloc_data = mem::data_pointer(alloc_header, request.alignment);
            alloc_data_end = ice::ptr_add(alloc_data, request.size);
        }

        if (is_locked(alloc_data_end))
        {
            return _backing_alloc.allocate(request);
        }

        _allocate = alloc_data_end;
        mem::fill(alloc_header, alloc_data, ice::ptr_distance(alloc_header, alloc_data_end), request.size);
        return { .memory = alloc_data, .size = request.size, .alignment = request.alignment };
    }

    void RingAllocator::do_deallocate(void* pointer) noexcept
    {
        if (is_backed(pointer))
        {
            _backing_alloc.deallocate(pointer);
            return;
        }

        // Get the associated allocation header
        mem::AllocationHeader* alloc_header = mem::header(pointer);

        ICE_ASSERT_CORE((alloc_header->allocated_size.value & detail::Constant_FreeMemoryBit) == 0);
        alloc_header->allocated_size.value = alloc_header->allocated_size.value | detail::Constant_FreeMemoryBit;

        // We don't need 'h' anymore.
        alloc_header = nullptr;

        // First we need to check if we even can write into the header!
        if (ice::ptr_distance(_free, _end) <= ice::size_of<mem::AllocationHeader>)
        {
            _free = _begin;
        }

        // Advance the free pointer past all free slots.
        while (_free != _allocate)
        {
            alloc_header = detail::get_header(_free);

            // Until we find an locked memory segment.
            if ((alloc_header->allocated_size.value & detail::Constant_FreeMemoryBit) == 0)
            {
                break;
            }

            // Move the free pointer by the given amount of bytes.
            _free = ice::ptr_add(alloc_header, { alloc_header->allocated_size.value & detail::Constant_FreeMemoryMask });
            if (ptr_distance(_free, _end) <= ice::size_of<mem::AllocationHeader>)
            {
                _free = _begin;

                // If the next allocation is above we can safely move it as it would do it anyway on the next allocation.
                // This alos prevents a infinite loop when we have an empty scratch allocator.
                // If the _allocate value would be higher or equal to _free it would hit the loop condition.
                if (_allocate >= _free)
                {
                    _allocate = _begin;
                }
            }
        }
    }

    bool RingAllocator::is_locked(void* pointer) const noexcept
    {
        if (_free == _allocate) // TODO: Check if this is needed.
        {
            return false;
        }
        if (_allocate > _free)
        {
            return pointer >= _free && pointer < _allocate;
        }
        return pointer >= _free || pointer < _allocate;
    }

    bool RingAllocator::is_backed(void* pointer) const noexcept
    {
        return pointer < _begin || pointer >= _end;
    }

    //auto RingAllocator::allocated_size(void* pointer) const noexcept -> uint32_t
    //{
    //    if (is_backing_pointer(pointer))
    //    {
    //        return _backing.allocated_size(pointer);
    //    }
    //    else
    //    {
    //        tracking::AllocationHeader const* alloc_header = tracking::header(pointer);
    //        if ((alloc_header->allocated_size & detail::Constant_FreeMemoryBit) == 0)
    //        {
    //            return alloc_header->requested_size;
    //        }
    //        else
    //        {
    //            return Constant_SizeNotTracked;
    //        }
    //    }
    //}

    //auto RingAllocator::total_allocated() const noexcept -> uint32_t
    //{
    //    auto distance = ptr_distance(_free, _allocate);
    //    if (distance < 0)
    //    {
    //        distance += ptr_distance(_begin, _end);
    //    }
    //    return distance;
    //}

    void RingAllocator::reset() noexcept
    {
        if constexpr (ice::Allocator::HasDebugInformation)
        {
            ICE_ASSERT_CORE(this->allocation_count() == 0);
            ICE_ASSERT_CORE(this->allocation_size_inuse() == 0_B);
        }

        // Always reset pointers on the allocator!
        _allocate = _begin;
        _free = _begin;
    }

} // namespace ice
