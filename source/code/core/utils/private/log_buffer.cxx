#include "log_buffer.hxx"

namespace ice::detail
{

    LogMessageBuffer::LogMessageBuffer(ice::Allocator& alloc, ice::u32 initial_allocation) noexcept
        : fmt::detail::buffer<char>{ }
        , _allocator{ alloc }
    {
        this->set(
            reinterpret_cast<char*>(_allocator.allocate(initial_allocation)),
            initial_allocation
        );
    }

    LogMessageBuffer::~LogMessageBuffer() noexcept
    {
        void* data = this->data();
        _allocator.deallocate(data);
    }

    void LogMessageBuffer::grow(uint64_t size) noexcept
    {
        size_t old_capacity = this->capacity();
        size_t new_capacity = old_capacity + old_capacity / 2;

        if (size > new_capacity)
        {
            new_capacity = size;
        }

        void* old_data = this->data();
        char* new_data = reinterpret_cast<char*>(_allocator.allocate(new_capacity));

        // The following code doesn't throw, so the raw pointer above doesn't leak.
        ice::memcpy(new_data, old_data, old_capacity);

        this->set(new_data, new_capacity);

        // deallocate must not throw according to the standard, but even if it does,
        // the buffer already uses the new storage and will deallocate it in
        // destructor.
        _allocator.deallocate(old_data);
    }

} // namespace ice::detail
