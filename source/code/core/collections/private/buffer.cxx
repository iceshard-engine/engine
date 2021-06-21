#include <ice/buffer.hxx>

namespace ice
{

    Buffer::Buffer(Buffer&& other) noexcept
        : _allocator{ other._allocator }
        , _size{ ice::exchange(other._size, 0) }
        , _capacity{ ice::exchange(other._capacity, 0) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    Buffer::Buffer(Buffer const& other) noexcept
        : _allocator{ other._allocator }
    {
        if (other._size > 0)
        {
            ice::buffer::set_capacity(*this, other._capacity);
            ice::memcpy(_data, other._data, other._size);
            _size = other._size;
        }
    }

    Buffer::~Buffer()
    {
        _allocator->deallocate(_data);
    }

    Buffer::Buffer(ice::Allocator& alloc) noexcept
        : _allocator{ &alloc }
    {
    }

    Buffer::Buffer(ice::Allocator& alloc, ice::Data data) noexcept
        : _allocator{ &alloc }
    {
        ice::buffer::append(*this, data);
    }

    auto Buffer::operator=(Buffer&& other) noexcept -> Buffer&
    {
        if (this != &other)
        {
            ice::swap(_allocator, other._allocator);
            ice::swap(_capacity, other._capacity);
            ice::swap(_data, other._data);
            _size = ice::exchange(other._size, 0);
        }
        return *this;
    }

    auto Buffer::operator=(Buffer const& other) noexcept -> Buffer&
    {
        if (this != &other)
        {
            ice::buffer::reserve(*this, other._capacity);
            if (other._size > 0)
            {
                ice::memcpy(_data, other._data, other._size);
            }
            _size = other._size;
        }
        return *this;
    }

    namespace buffer
    {

        auto size(ice::Buffer const& buffer) noexcept -> u32
        {
            return buffer._size;
        }

        auto capacity(ice::Buffer const& buffer) noexcept -> u32
        {
            return buffer._capacity;
        }

        bool empty(ice::Buffer const& buffer) noexcept
        {
            return buffer._size == 0;
        }

        auto data(ice::Buffer const& buffer) noexcept -> void const*
        {
            return buffer._data;
        }


        void set_capacity(ice::Buffer& buffer, u32 new_capacity) noexcept
        {
            if (new_capacity == buffer._capacity)
            {
                return;
            }

            if (new_capacity < buffer._size)
            {
                buffer._size = new_capacity;
            }

            void* new_data = nullptr;
            if (new_capacity > 0)
            {
                new_data = buffer._allocator->allocate(new_capacity);
                if (buffer._size > 0)
                {
                    ice::memcpy(new_data, buffer._data, buffer._size);
                }
            }

            buffer._allocator->deallocate(buffer._data);
            buffer._data = new_data;
            buffer._capacity = new_capacity;
        }

        void set_capacity_aligned(ice::Buffer& buffer, u32 new_capacity, u32 alignment) noexcept
        {
            if (new_capacity == buffer._capacity)
            {
                return;
            }

            if (new_capacity < buffer._size)
            {
                buffer._size = new_capacity;
            }

            void* new_data = nullptr;
            if (new_capacity > 0)
            {
                new_data = buffer._allocator->allocate(new_capacity, alignment);
                if (buffer._size > 0)
                {
                    ice::memcpy(new_data, buffer._data, buffer._size);
                }
            }

            buffer._allocator->deallocate(buffer._data);
            buffer._data = new_data;
            buffer._capacity = new_capacity;
        }

        void reserve(ice::Buffer& buffer, u32 min_capacity) noexcept
        {
            if (buffer._capacity < min_capacity)
            {
                ice::buffer::set_capacity(buffer, min_capacity);
            }
        }

        void grow(ice::Buffer& buffer, u32 min_capacity) noexcept
        {
            uint32_t new_capacity = buffer._capacity * 2 + 8;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }

            ice::buffer::set_capacity(buffer, new_capacity);
        }

        void shrink(ice::Buffer& buffer) noexcept
        {
            ice::buffer::set_capacity(buffer, buffer._size);
        }

        void resize(ice::Buffer& buffer, u32 new_size) noexcept
        {
            if (buffer._capacity < new_size)
            {
                ice::buffer::set_capacity(buffer, new_size);
            }
            buffer._size = new_size;
        }

        void clear(ice::Buffer& buffer) noexcept
        {
            buffer._size = 0;
        }

        auto data(ice::Buffer& buffer) noexcept -> void*
        {
            return buffer._data;
        }

        auto append(ice::Buffer& buffer, Data data) noexcept -> void*
        {
            uint32_t const required_capacity = buffer._size + data.size + data.alignment;
            if (buffer._capacity < required_capacity)
            {
                ice::buffer::grow(buffer, required_capacity);
            }

            void* dest_data = ice::memory::ptr_add(buffer._data, buffer._size);
            dest_data = ice::memory::ptr_align_forward(dest_data, data.alignment);

            if (data.location != nullptr)
            {
                ice::memcpy(dest_data, data.location, data.size);
            }

            buffer._size = ice::memory::ptr_distance(buffer._data, dest_data) + data.size;
            return dest_data;
        }

        auto append(ice::Buffer& buffer, void const* data, u32 size, u32 alignment) noexcept -> void*
        {
            return ice::buffer::append(buffer, ice::data_view(data, size, alignment));
        }

        auto extrude_memory(ice::Buffer& buffer) noexcept -> ice::Memory
        {
            buffer._capacity = 0;

            return ice::Memory{
                .location = ice::exchange(buffer._data, nullptr),
                .size = ice::exchange(buffer._size, 0),
                .alignment = ice::Allocator::Constant_DefaultAlignment,
            };
        }

    } // namespace buffer

} // namespace ice
