#pragma once
#include <memsys/memsys.hxx>

namespace memsys::memory_tracking
{

//! \brief Allocation header structure used for memory tracking.
struct allocation_header
{
    //! \brief The requested allocation size.
    uint32_t size;
};

//! \brief If we need to align the memory allocation we pad the header with this
//!     value after storing the size. That way we can find the pointer header.
//!
//! \remarks The value is 4 bytes because thats the lowest possible alignment.
static constexpr uint32_t HEADER_PAD_VALUE = 0xffffffffu;

//! \brief Returns the data pointer from the given header.
//!
//! \param [in] header The allocation header.
//! \param [in] align The alignment of the allocation.
inline void* data_pointer(allocation_header* const header, uint32_t alignment) noexcept
{
    return utils::align_forward(header + 1, alignment);
}

//! \brief Returns the allocation header from the given data pointer.
inline auto header(void* const data) noexcept -> allocation_header*
{
    auto* temp_pointer = reinterpret_cast<uint32_t*>(data);

    // Subtract the pointer for every HEADER_PAD_VALUE we encounter.
    while (temp_pointer[-1] == HEADER_PAD_VALUE)
    {
        --temp_pointer;
    }

    // Return the pointer subtracted by the size of the allocation header.
    return reinterpret_cast<allocation_header*>(utils::pointer_sub(temp_pointer, sizeof(allocation_header)));
}

// \brief Stores the size in the header and pads with HEADER_PAD_VALUE up to the data pointer.
inline void fill(allocation_header* header, void* data_pointer, uint32_t size) noexcept
{
    header->size = size;

    auto* header_pointer = reinterpret_cast<uint32_t*>(header + 1);
    while (header_pointer < data_pointer)
    {
        *header_pointer = HEADER_PAD_VALUE;
        header_pointer += 1;
    }
}

} // namespace memsys
