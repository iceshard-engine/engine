#pragma once
#include <ice/allocator.hxx>
#include <ice/memory/pointer_arithmetic.hxx>
//#include <core/debug/assert.hxx>
#include <cassert>

namespace ice::memory
{

    //! \brief This allocator consumes the given amount of memory on the stack and advances on allocation requests.
    //! \remarks Allocation is not tracked.
    template<uint32_t BufferSize>
    class StackAllocator final : public ice::Allocator
    {
    public:
        static constexpr uint32_t Constant_BufferSize = BufferSize;

        StackAllocator() noexcept;
        ~StackAllocator() noexcept override = default;

        //! \copydoc allocator::allocate(uint32_t size, uint32_t align)
        auto allocate(uint32_t size, uint32_t align = Constant_DefaultAlignment) noexcept -> void* override;

        //! \copydoc allocator::deallocate(void* ptr)
        void deallocate(void* ptr) noexcept override;

        //! \copydoc allocator::allocated_size(void* ptr)
        auto allocated_size(void* ptr) noexcept -> uint32_t override;

        //! \copydoc allocator::total_allocated
        auto total_allocated() noexcept -> uint32_t override;

        //! \brief Resets the allocator internal State.
        void clear() noexcept;

    private:
        //! \brief The allocator available memory.
        std::byte _static_memory[Constant_BufferSize];

        //! \brief Next free memory location.
        void* _next_free;
    };


    //! \brief Predefined stack allocator for 512 bytes.
    using StackAllocator_512 = StackAllocator<512>;

    //! \brief Predefined stack allocator for 1 kilobytes.
    using StackAllocator_1024 = StackAllocator<1024>;

    //! \brief Predefined stack allocator for 2 kilobytes.
    using StackAllocator_2048 = StackAllocator<2048>;

    //! \brief Predefined stack allocator for 4 kilobytes.
    using StackAllocator_4096 = StackAllocator<4096>;


    template<uint32_t BufferSize>
    StackAllocator<BufferSize>::StackAllocator() noexcept
        : ice::Allocator{ }
        , _static_memory{ }
        , _next_free{ _static_memory }
    { }

    template<uint32_t BufferSize>
    auto StackAllocator<BufferSize>::allocate(uint32_t size, uint32_t align /*= Constant_DefaultAlignment*/) noexcept -> void*
    {
        void* free_location = ptr_align_forward(_next_free, align);
        _next_free = ptr_add(free_location, size);

        assert(ptr_distance(_static_memory, _next_free) <= Constant_BufferSize);/* ,
            "Stack allocator overgrown by {} bytes!",
            utils::pointer_distance(_static_memory, _next_free)
        );*/
        return free_location;
    }

    template<uint32_t BufferSize>
    void StackAllocator<BufferSize>::deallocate(void*) noexcept
    {
        /* not implemented */
    }

    template<uint32_t BufferSize>
    auto StackAllocator<BufferSize>::allocated_size(void*) noexcept -> uint32_t
    {
        return Constant_SizeNotTracked;
    }

    template<uint32_t BufferSize>
    auto StackAllocator<BufferSize>::total_allocated() noexcept -> uint32_t
    {
        return Constant_SizeNotTracked;
    }

    template<uint32_t BufferSize>
    void StackAllocator<BufferSize>::clear() noexcept
    {
        _next_free = _static_memory;
    }


} // namespace ice::memory
