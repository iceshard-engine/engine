#pragma once
#include <ice/allocator.hxx>
#include <ice/data.hxx>
#include <ice/memory.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice
{

    struct Buffer final
    {
        Buffer(Buffer&& other) noexcept;
        Buffer(Buffer const& other) noexcept;
        ~Buffer() noexcept;

        Buffer(ice::Allocator& alloc) noexcept;
        Buffer(ice::Allocator& alloc, ice::Data data) noexcept;

        auto operator=(Buffer&& other) noexcept -> Buffer&;
        auto operator=(Buffer const& other) noexcept -> Buffer&;

        operator ice::Data() const noexcept;

        ice::Allocator* _allocator;
        ice::u32 _size = 0;
        ice::u32 _capacity = 0;
        void* _data = nullptr;
    };

    namespace buffer
    {

        auto size(ice::Buffer const& buffer) noexcept -> u32;

        auto capacity(ice::Buffer const& buffer) noexcept -> u32;

        bool empty(ice::Buffer const& buffer) noexcept;

        auto data(ice::Buffer const& buffer) noexcept -> void const*;


        void set_capacity(ice::Buffer& buffer, u32 new_capacity) noexcept;

        void set_capacity_aligned(ice::Buffer& buffer, u32 new_capacity, u32 alignment) noexcept;

        void reserve(ice::Buffer& buffer, u32 min_capacity) noexcept;

        void grow(ice::Buffer& buffer, u32 min_capacity = 0) noexcept;

        void shrink(ice::Buffer& buffer) noexcept;

        void resize(ice::Buffer& buffer, u32 new_size) noexcept;

        void clear(ice::Buffer& buffer) noexcept;

        auto data(ice::Buffer& buffer) noexcept -> void*;

        auto append(ice::Buffer& buffer, ice::Data data) noexcept -> void*;

        auto append(ice::Buffer& buffer, const void* data, u32 size, u32 alignment) noexcept -> void*;

        auto extrude_memory(ice::Buffer& buffer) noexcept -> ice::Memory;

    } // namespace buffer

    inline Buffer::operator ice::Data() const noexcept
    {
        return Data{
            .location = _data,
            .size = _size,
            .alignment = ice::Allocator::Constant_DefaultAlignment
        };
    }

} // namespace ice
