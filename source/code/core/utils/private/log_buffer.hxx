/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <fmt/format.h>

namespace ice::detail
{

    class LogMessageBuffer final : public fmt::v11::detail::buffer<char>
    {
    public:
        LogMessageBuffer(ice::Allocator& alloc, ice::ucount initial_allocation) noexcept;
        ~LogMessageBuffer() noexcept;

        void grow(size_t size) noexcept;

    private:
        ice::Allocator& _allocator;
    };

    static thread_local ice::StackAllocator_2048 log_buffer_alloc{ };

} // ice::detail
