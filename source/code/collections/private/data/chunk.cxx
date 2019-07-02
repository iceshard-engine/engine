#include <core/data/chunk.hxx>

namespace core
{

    data_chunk::data_chunk(core::allocator & alloc, uint32_t size) noexcept
        : _allocator{ &alloc }
        , _data{ nullptr }
        , _size{ size }
    {
        _data = _allocator->allocate(size);
    }

    data_chunk::data_chunk(core::allocator& alloc, void* data, uint32_t size) noexcept
        : _allocator{ &alloc }
        , _data{ data }
        , _size{ size }
    {
    }

    data_chunk::data_chunk(core::allocator& alloc, data_view data) noexcept
        : _allocator{ &alloc }
        , _data{ _allocator->allocate(data._size) }
        , _size{ data._size }
    {
        std::memcpy(_data, data._data, data._size);
    }

    data_chunk::data_chunk(data_chunk&& other) noexcept
        : _allocator{ other._allocator }
        , _data{ other._data }
        , _size{ other._size }
    {
        other._data = nullptr;
        other._size = 0;
    }

    data_chunk::data_chunk(const data_chunk& other) noexcept
        : _allocator{ other._allocator }
        , _data{ _allocator->allocate(other._size) }
        , _size{ other._size }
    {
        std::memcpy(_data, other._data, other._size);
    }

    data_chunk::~data_chunk() noexcept
    {
        _allocator->deallocate(_data);
    }

    auto data_chunk::operator=(data_chunk&& other) noexcept -> data_chunk&
    {
        if (this == &other)
        {
            return *this;
        }

        if (_data != nullptr)
        {
            _allocator->deallocate(_data);
        }

        _allocator = other._allocator;
        _data = other._data;
        _size = other._size;
        other._data = nullptr;
        return *this;
    }

    auto data_chunk::operator=(const data_chunk& other) noexcept -> data_chunk&
    {
        if (this == &other)
        {
            return *this;
        }

        if (_data != nullptr)
        {
            _allocator->deallocate(_data);
        }

        _data = _allocator->allocate(other._size);
        memcpy(_data, other._data, other._size);

        _size = other._size;
        return *this;
    }

} // namespace core
