#pragma once
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice::memory::tracking
{

    struct AllocationHeader
    {
        uint32_t requested_size;
        uint32_t allocated_size;
    };

    static constexpr uint32_t HeaderPadValue = 0xffff'ffffu;

    inline void* data_pointer(AllocationHeader* const header, uint32_t alignment) noexcept
    {
        return ice::memory::ptr_align_forward(header + 1, alignment);
    }

    inline auto header(void* const data) noexcept -> AllocationHeader*
    {
        auto* temp_pointer = reinterpret_cast<uint32_t*>(data);

        // Subtract the pointer for every HEADER_PAD_VALUE we encounter.
        while (temp_pointer[-1] == HeaderPadValue)
        {
            --temp_pointer;
        }

        // Return the pointer subtracted by the size of the allocation header.
        return reinterpret_cast<AllocationHeader*>(
            ice::memory::ptr_sub(temp_pointer, sizeof(AllocationHeader))
        );
    }

    // \brief Stores the size in the header and pads with HEADER_PAD_VALUE up to the data pointer.
    inline void fill(
        AllocationHeader* header,
        void* data_pointer,
        uint32_t allocated_size,
        uint32_t requested_size
    ) noexcept
    {
        header->allocated_size = allocated_size;
        header->requested_size = requested_size;

        auto* header_pointer = reinterpret_cast<uint32_t*>(header + 1);
        while (header_pointer < data_pointer)
        {
            *header_pointer = HeaderPadValue;
            header_pointer += 1;
        }
    }

} // namespace ice::memory::tracking
