//#include <ice/buffer.hxx>
//
//namespace ice
//{
//
//    Buffer::Buffer(Buffer&& other) noexcept
//        : _allocator{ other._allocator }
//        , _size{ ice::exchange(other._size, ice::usize{}) }
//        , _capacity{ ice::exchange(other._capacity, ice::usize{}) }
//        , _data{ ice::exchange(other._data, nullptr) }
//    {
//    }
//
//    Buffer::Buffer(Buffer const& other) noexcept
//        : _allocator{ other._allocator }
//    {
//        if (other._size > 0_B)
//        {
//            ice::buffer::set_capacity(*this, other._capacity);
//            ice::memcpy(_data, other._data, other._size);
//            _size = other._size;
//        }
//    }
//
//    Buffer::~Buffer()
//    {
//        _allocator->deallocate({ _data });
//    }
//
//    Buffer::Buffer(ice::Allocator& alloc) noexcept
//        : _allocator{ &alloc }
//    {
//    }
//
//    Buffer::Buffer(ice::Allocator& alloc, ice::Data data) noexcept
//        : _allocator{ &alloc }
//    {
//        ice::buffer::append(*this, data);
//    }
//
//    auto Buffer::operator=(Buffer&& other) noexcept -> Buffer&
//    {
//        if (this != &other)
//        {
//            ice::swap(_allocator, other._allocator);
//            ice::swap(_capacity, other._capacity);
//            ice::swap(_data, other._data);
//            _size = ice::exchange(other._size, ice::usize{});
//        }
//        return *this;
//    }
//
//    auto Buffer::operator=(Buffer const& other) noexcept -> Buffer&
//    {
//        if (this != &other)
//        {
//            ice::buffer::reserve(*this, other._capacity);
//            if (other._size > 0_B)
//            {
//                ice::memcpy(_data, other._data, other._size);
//            }
//            _size = other._size;
//        }
//        return *this;
//    }
//
//    namespace buffer
//    {
//
//        auto size(ice::Buffer const& buffer) noexcept -> ice::usize
//        {
//            return buffer._size;
//        }
//
//        auto capacity(ice::Buffer const& buffer) noexcept -> ice::usize
//        {
//            return buffer._capacity;
//        }
//
//        bool empty(ice::Buffer const& buffer) noexcept
//        {
//            return buffer._size == 0_B;
//        }
//
//        auto data(ice::Buffer const& buffer) noexcept -> void const*
//        {
//            return buffer._data;
//        }
//
//
//        void set_capacity(ice::Buffer& buffer, ice::usize new_capacity) noexcept
//        {
//            if (new_capacity == buffer._capacity)
//            {
//                return;
//            }
//
//            if (new_capacity < buffer._size)
//            {
//                buffer._size = new_capacity;
//            }
//
//            void* new_data = nullptr;
//            if (new_capacity > 0_B)
//            {
//                new_data = buffer._allocator->allocate(new_capacity).result;
//                if (buffer._size > 0_B)
//                {
//                    ice::memcpy(new_data, buffer._data, buffer._size);
//                }
//            }
//
//            buffer._allocator->deallocate({ buffer._data });
//            buffer._data = new_data;
//            buffer._capacity = new_capacity;
//        }
//
//        void set_capacity_aligned(ice::Buffer& buffer, ice::usize new_capacity, ice::ualign alignment) noexcept
//        {
//            if (new_capacity == buffer._capacity)
//            {
//                return;
//            }
//
//            if (new_capacity < buffer._size)
//            {
//                buffer._size = new_capacity;
//            }
//
//            void* new_data = nullptr;
//            if (new_capacity > 0_B)
//            {
//                new_data = buffer._allocator->allocate({ new_capacity, alignment }).result;
//                if (buffer._size > 0_B)
//                {
//                    ice::memcpy(new_data, buffer._data, buffer._size);
//                }
//            }
//
//            buffer._allocator->deallocate({ buffer._data });
//            buffer._data = new_data;
//            buffer._capacity = new_capacity;
//        }
//
//        void reserve(ice::Buffer& buffer, ice::usize min_capacity) noexcept
//        {
//            if (buffer._capacity < min_capacity)
//            {
//                ice::buffer::set_capacity(buffer, min_capacity);
//            }
//        }
//
//        void grow(ice::Buffer& buffer, ice::usize min_capacity) noexcept
//        {
//            ice::usize new_capacity = buffer._capacity * 2 + 8_B;
//            if (new_capacity < min_capacity)
//            {
//                new_capacity = min_capacity;
//            }
//
//            ice::buffer::set_capacity(buffer, new_capacity);
//        }
//
//        void shrink(ice::Buffer& buffer) noexcept
//        {
//            ice::buffer::set_capacity(buffer, buffer._size);
//        }
//
//        void resize(ice::Buffer& buffer, ice::usize new_size) noexcept
//        {
//            if (buffer._capacity < new_size)
//            {
//                ice::buffer::set_capacity(buffer, new_size);
//            }
//            buffer._size = new_size;
//        }
//
//        void clear(ice::Buffer& buffer) noexcept
//        {
//            buffer._size = 0_B;
//        }
//
//        auto data(ice::Buffer& buffer) noexcept -> void*
//        {
//            return buffer._data;
//        }
//
//        auto append(ice::Buffer& buffer, ice::Data data) noexcept -> void*
//        {
//            // TODO: Check if we can make this calculation prettier, or if we even need this type.
//            ice::usize const required_capacity = buffer._size + data.size + ice::usize{ static_cast<ice::u32>(data.alignment) };
//            if (buffer._capacity < required_capacity)
//            {
//                ice::buffer::grow(buffer, required_capacity);
//            }
//
//            ice::AlignResult const alignres = ice::align_to(
//                ice::ptr_add(buffer._data, buffer._size),
//                data.alignment
//            );
//
//            if (data.location != nullptr)
//            {
//                ice::memcpy(alignres.value, data.location, data.size);
//            }
//
//            ice::isize const new_size = ice::ptr_distance(buffer._data, alignres.value) + data.size;
//
//            // We explicitly create 'usize' from 'isize', as we have ensured the distance is always positive.
//            buffer._size = ice::usize{ static_cast<ice::usize::base_type>(new_size.value) };
//            return alignres.value;
//        }
//
//        auto append(ice::Buffer& buffer, void const* data, ice::usize size, ice::ualign alignment) noexcept -> void*
//        {
//            return ice::buffer::append(buffer, { data, size, alignment });
//        }
//
//        auto extrude_memory(ice::Buffer& buffer) noexcept -> ice::Memory
//        {
//            buffer._capacity = 0_B;
//
//            return ice::Memory{
//                .location = ice::exchange(buffer._data, nullptr),
//                .size = ice::exchange(buffer._size, ice::usize{}),
//                .alignment = ice::ualign::b_default,
//            };
//        }
//
//    } // namespace buffer
//
//} // namespace ice
