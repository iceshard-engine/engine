#pragma once
#include <ice/allocator.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <fmt/format.h>

namespace ice::detail
{

    class LogMessageBuffer final : public fmt::detail::buffer<char>
    {
    public:
        LogMessageBuffer(ice::Allocator& alloc, ice::u32 initial_allocation) noexcept;

        ~LogMessageBuffer() noexcept override;

        void grow(uint64_t size) noexcept override;

    private:
        ice::Allocator& _allocator;
    };

    static thread_local ice::memory::StackAllocator_2048 log_buffer_alloc{ };

} // ice::detail
