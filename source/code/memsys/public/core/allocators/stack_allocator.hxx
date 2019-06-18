#pragma once
#include <core/memory.hxx>
#include <core/debug/assert.hxx>

namespace core::memory
{


//! \brief This allocator consumes the given amount of memory on the stack and advances on allocation requests.
//! \remarks Allocation is not tracked.
template<uint32_t BUFFER_SIZE>
class stack_allocator : public core::allocator
{
public:
    stack_allocator() noexcept;
    ~stack_allocator() noexcept override = default;

    //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
    auto allocate(uint32_t size, uint32_t align = DEFAULT_ALIGN) noexcept -> void* override;

    //! \copydoc allocator::deallocate(void* ptr)
    void deallocate(void* ptr) noexcept override;

    //! \copydoc allocator::allocated_size(void* ptr)
    auto allocated_size(void* ptr) noexcept -> uint32_t override;

    //! \copydoc allocator::total_allocated
    auto total_allocated() noexcept -> uint32_t override;

    //! \brief Resets the allocator internal state.
    void clear() noexcept;

private:
    //! \brief The allocator available memory.
    char _static_memory[BUFFER_SIZE];

    //! \brief Next free memory location.
    void* _next_free;
};


//! \brief Predefined stack allocator for 512 bytes.
using stack_allocator_512 = stack_allocator<512>;

//! \brief Predefined stack allocator for 1 kilobytes.
using stack_allocator_1024 = stack_allocator<1024>;

//! \brief Predefined stack allocator for 2 kilobytes.
using stack_allocator_2048 = stack_allocator<2048>;

//! \brief Predefined stack allocator for 4 kilobytes.
using stack_allocator_4096 = stack_allocator<4096>;


#include "stack_allocator.inl"


} // namespace core::memory
