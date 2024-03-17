/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_buffer.hxx"

namespace ice::detail
{

    LogMessageBuffer::LogMessageBuffer(ice::Allocator& alloc, ice::ucount initial_allocation) noexcept
        : fmt::detail::buffer<char>{ }
        , _allocator{ alloc }
    {
        this->set(
            _allocator.allocate<char>(initial_allocation),
            initial_allocation
        );
    }

    LogMessageBuffer::~LogMessageBuffer() noexcept
    {
        _allocator.deallocate(
            Memory{
                .location = this->data(),
                .size = { this->capacity() },
                .alignment = align_of<char>
            }
        );
    }

    void LogMessageBuffer::grow(size_t min_size) noexcept
    {
        ice::usize::base_type const old_capacity = this->capacity();
        ice::usize::base_type const new_capacity = ice::max(old_capacity + old_capacity / 2, min_size);

        void* const old_data = this->data();
        char* const new_data = _allocator.allocate<char>(new_capacity);

        ice::memcpy(new_data, old_data, old_capacity);
        this->set(new_data, new_capacity);

        // deallocate must not throw according to the standard, but even if it does,
        // the buffer already uses the new storage and will deallocate it in
        // destructor.
        _allocator.deallocate(
            Memory{
                .location = old_data,
                .size = { old_capacity },
                .alignment = align_of<char>
            }
        );
    }

} // namespace ice::detail
