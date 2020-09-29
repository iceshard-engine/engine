#include <core/data/buffer.hxx>
#include <core/debug/assert.hxx>
#include <core/memory.hxx>

#include <cassert>

namespace core
{

    Buffer::Buffer(core::allocator& alloc) noexcept
        : _allocator{ &alloc }
    {
    }

    Buffer::Buffer(core::allocator& alloc, data_view data) noexcept
        : _allocator{ &alloc }
    {
        buffer::append(*this, data.data(), data.size());
    }

    Buffer::~Buffer()
    {
        _allocator->deallocate(_data);
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : _allocator{ other._allocator }
        , _size{ std::exchange(other._size, 0) }
        , _capacity{ std::exchange(other._capacity, 0) }
        , _data{ std::exchange(other._allocator, nullptr) }
    {
    }

    Buffer::Buffer(const Buffer& other) noexcept
        : _allocator{ other._allocator }
    {
        const auto other_size = other._size;
        buffer::set_capacity(*this, other_size);
        memcpy(_data, other._data, other_size);
        _size = other_size;
    }

    auto Buffer::operator=(Buffer&& other) noexcept -> Buffer&
    {
        if (this != &other)
        {
            std::swap(_allocator, other._allocator);
            std::swap(_data, other._data);
            std::swap(_size, other._size);
            std::swap(_capacity, other._capacity);
        }
        return *this;
    }

    auto Buffer::operator=(const Buffer& other) noexcept -> Buffer&
    {
        if (this == &other)
        {
            return *this;
        }

        const uint32_t n = other._size;
        buffer::resize(*this, n);
        memcpy(_data, other._data, n);
        return *this;
    }

    auto buffer::size(const Buffer& b) noexcept -> uint32_t
    {
        return b._size;
    }

    auto buffer::capacity(const Buffer& b) noexcept -> uint32_t
    {
        return b._capacity;
    }

    bool buffer::empty(const Buffer& b) noexcept
    {
        return b._size == 0;
    }

    auto buffer::data(const Buffer& b) noexcept -> void*
    {
        return b._data;
    }

    void buffer::clear(Buffer& b) noexcept
    {
        resize(b, 0);
    }

    void buffer::trim(Buffer& b) noexcept
    {
        set_capacity(b, b._size);
    }

    auto buffer::append(Buffer& b, const void* data, uint32_t size) noexcept -> void*
    {
        const uint32_t new_size = b._size + size;

        if (new_size > b._capacity)
        {
            grow(b, new_size);
        }

        IS_ASSERT(new_size <= b._capacity, "Couldn't reserve enough memory for the new buffer size! [ size:{}, capacity:{} ]", new_size, b._capacity);

        void* const buffer_end = core::memory::utils::pointer_add(b._data, b._size);
        if (data != nullptr)
        {
            memcpy(buffer_end, data, size);
        }

        b._size = new_size;
        return buffer_end;
    }

    auto buffer::append(Buffer& b, data_view data) noexcept -> void*
    {
        return append(b, data._data, data._size);
    }

    auto buffer::append_aligned(Buffer& b, const void* data, uint32_t size, uint32_t align) noexcept -> void*
    {
        const uint32_t old_size_aligned = (b._size % align) == 0 ? (b._size) : (b._size + (align - (b._size % align)));
        const uint32_t new_size = old_size_aligned + size;

        if (new_size > b._capacity)
        {
            grow(b, new_size);
        }

        IS_ASSERT(new_size <= b._capacity, "Couldn't reserve enough memory for the new buffer size! [ size:{}, capacity:{} ]", new_size, b._capacity);

        void* const buffer_end = core::memory::utils::pointer_add(b._data, old_size_aligned);
        if (data != nullptr)
        {
            memcpy(buffer_end, data, size);
        }

        b._size = new_size;
        return buffer_end;
    }

    auto buffer::append_aligned(Buffer& b, data_view_aligned data) noexcept -> void*
    {
        return append_aligned(b, data._data, data._size, data._align);
    }

    void buffer::resize(Buffer& b, uint32_t new_size) noexcept
    {
        if (new_size > b._capacity)
        {
            grow(b, new_size);
        }
        b._size = new_size;
    }

    void buffer::reserve(Buffer& b, uint32_t new_capacity) noexcept
    {
        if (new_capacity > b._capacity)
        {
            grow(b, new_capacity);
        }
    }

    void buffer::set_capacity(Buffer& b, uint32_t new_capacity) noexcept
    {
        if (new_capacity == b._capacity)
        {
            return;
        }

        if (new_capacity < b._size)
        {
            resize(b, new_capacity);
        }

        void* new_data = nullptr;
        if (new_capacity > 0)
        {
            new_data = b._allocator->allocate(new_capacity);
            memcpy(new_data, b._data, b._size);
        }

        b._allocator->deallocate(b._data);
        b._data = new_data;
        b._capacity = new_capacity;
    }

    void buffer::grow(Buffer& b, uint32_t min_capacity /*= 0*/) noexcept
    {
        uint32_t new_capacity = b._capacity * 2 + 8;
        if (new_capacity < min_capacity)
        {
            new_capacity = min_capacity;
        }
        set_capacity(b, new_capacity);
    }

    auto buffer::begin(const Buffer& b) noexcept -> const void*
    {
        return b._data;
    }

    auto buffer::begin(Buffer& b) noexcept -> void*
    {
        return b._data;
    }

    auto buffer::end(const Buffer& b) noexcept -> const void*
    {
        return core::memory::utils::pointer_add(b._data, b._size);
    }

    auto buffer::end(Buffer& b) noexcept -> void*
    {
        return core::memory::utils::pointer_add(b._data, b._size);
    }


} // namespace core
