//#pragma once
//#include <ice/mem_allocator.hxx>
//#include <ice/mem_memory.hxx>
//#include <ice/mem_data.hxx>
//
//namespace ice
//{
//
//    struct Buffer final
//    {
//        Buffer(Buffer&& other) noexcept;
//        Buffer(Buffer const& other) noexcept;
//        ~Buffer() noexcept;
//
//        Buffer(ice::Allocator& alloc) noexcept;
//        Buffer(ice::Allocator& alloc, ice::Data data) noexcept;
//
//        auto operator=(Buffer&& other) noexcept -> Buffer&;
//        auto operator=(Buffer const& other) noexcept -> Buffer&;
//
//        operator ice::Data() const noexcept;
//
//        ice::Allocator* _allocator;
//        ice::usize _size = 0_B;
//        ice::usize _capacity = 0_B;
//        void* _data = nullptr;
//    };
//
//    namespace buffer
//    {
//
//        auto size(ice::Buffer const& buffer) noexcept -> ice::usize;
//
//        auto capacity(ice::Buffer const& buffer) noexcept -> ice::usize;
//
//        bool empty(ice::Buffer const& buffer) noexcept;
//
//        auto data(ice::Buffer const& buffer) noexcept -> void const*;
//
//
//        void set_capacity(ice::Buffer& buffer, ice::usize new_capacity) noexcept;
//
//        void set_capacity_aligned(ice::Buffer& buffer, ice::usize new_capacity, ice::ualign alignment) noexcept;
//
//        void reserve(ice::Buffer& buffer, ice::usize min_capacity) noexcept;
//
//        void grow(ice::Buffer& buffer, ice::usize min_capacity = 0_B) noexcept;
//
//        void shrink(ice::Buffer& buffer) noexcept;
//
//        void resize(ice::Buffer& buffer, ice::usize new_size) noexcept;
//
//        void clear(ice::Buffer& buffer) noexcept;
//
//        auto data(ice::Buffer& buffer) noexcept -> void*;
//
//        auto append(ice::Buffer& buffer, ice::Data data) noexcept -> void*;
//
//        auto append(ice::Buffer& buffer, const void* data, ice::usize size, ice::ualign alignment) noexcept -> void*;
//
//        auto extrude_memory(ice::Buffer& buffer) noexcept -> ice::Memory;
//
//    } // namespace buffer
//
//    inline Buffer::operator ice::Data() const noexcept
//    {
//        return Data{
//            .location = _data,
//            .size = _size,
//            .alignment = ice::ualign::b_default
//        };
//    }
//
//} // namespace ice
