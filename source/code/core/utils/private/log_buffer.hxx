#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <fmt/format.h>

namespace ice::detail
{

    class LogMessageBuffer final : public fmt::detail::buffer<char>
    {
    public:
        LogMessageBuffer(ice::Allocator& alloc, ice::ucount initial_allocation) noexcept;
        ~LogMessageBuffer() noexcept override;

        void grow(ice::u64 size) noexcept override;

    private:
        ice::Allocator& _allocator;
    };

    static thread_local ice::StackAllocator_2048 log_buffer_alloc{ };

} // ice::detail
