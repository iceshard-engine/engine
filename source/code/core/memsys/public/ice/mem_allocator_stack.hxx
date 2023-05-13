/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{
    template<ice::usize Capacity>
    struct StackAllocator;

    //using StackAllocator_256 = StackAllocator<256_B>;
    using StackAllocator_1024 = StackAllocator<1024_B>;
    using StackAllocator_2048 = StackAllocator<2048_B>;

    template<ice::usize Capacity>
    struct StackAllocator final : ice::Allocator
    {
        static constexpr ice::usize Constant_InternalCapacity = Capacity;

        inline StackAllocator(
            std::source_location = std::source_location::current()
        ) noexcept;

        inline StackAllocator(
            ice::Allocator& backing_allocator,
            std::source_location = std::source_location::current()
        ) noexcept;

        inline StackAllocator(
            ice::Allocator& backing_allocator,
            std::string_view name,
            std::source_location = std::source_location::current()
        ) noexcept;

        inline void reset() noexcept;

    protected:
        inline auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        inline void do_deallocate(void* memory) noexcept override;

    private:
        ice::Allocator* _backing_alloc;

        ice::usize _static_usage;
        char _static_buffer[Constant_InternalCapacity.value];
    };

    template<ice::usize Capacity>
    inline StackAllocator<Capacity>::StackAllocator(
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc }
        , _backing_alloc{ nullptr }
        , _static_usage{ 0_B }
    {
    }

    template<ice::usize Capacity>
    inline StackAllocator<Capacity>::StackAllocator(
        ice::Allocator& backing_allocator,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator }
        , _backing_alloc{ &backing_allocator }
        , _static_usage{ 0_B }
    {
    }

    template<ice::usize Capacity>
    inline StackAllocator<Capacity>::StackAllocator(
        ice::Allocator& backing_allocator,
        std::string_view name,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator, name }
        , _backing_alloc{ &backing_allocator }
        , _static_usage{ 0_B }
    {
    }

    template<ice::usize Capacity>
    inline void StackAllocator<Capacity>::reset() noexcept
    {
        _static_usage = 0_B;
    }

    template<ice::usize Capacity>
    inline auto StackAllocator<Capacity>::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        ice::AllocResult result{
            .memory = nullptr,
            .size = 0_B,
            .alignment = ice::ualign::invalid
        };

        ice::AlignResult<ice::usize> const aligned_usage = ice::align_to(_static_usage, request.alignment);
        if ((aligned_usage.value + request.size) <= Capacity)
        {
            _static_usage = aligned_usage.value + request.size;

            result.memory = ice::ptr_add(_static_buffer, aligned_usage.value);
            result.size = request.size;
            result.alignment = request.alignment;
        }
        else if (_backing_alloc != nullptr)
        {
            result = _backing_alloc->allocate(request);
        }
        return result;
    }

    template<ice::usize Capacity>
    inline void StackAllocator<Capacity>::do_deallocate(void* pointer) noexcept
    {
        if ((_static_buffer + 0) <= pointer && ice::ptr_add(_static_buffer, _static_usage) > pointer)
        {
            ICE_ASSERT_CORE(true); // nothing to do here
        }
        else if (_backing_alloc != nullptr)
        {
            _backing_alloc->deallocate(pointer);
        }
    }

} // namespace ice
