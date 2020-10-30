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
        operator ice::Memory() const noexcept;

        ice::Allocator* _allocator;
        ice::u32 _size = 0;
        ice::u32 _capacity = 0;
        void* _data = nullptr;
    };

    namespace buffer
    {

        auto size(Buffer const& buffer) noexcept -> u32;

        auto capacity(Buffer const& buffer) noexcept -> u32;

        bool empty(Buffer const& buffer) noexcept;

        auto data(Buffer const& buffer) noexcept -> void const*;


        void set_capacity(Buffer& buffer, u32 new_capacity) noexcept;

        void reserve(Buffer& buffer, u32 min_capacity) noexcept;

        void grow(Buffer& buffer, u32 min_capacity = 0) noexcept;

        void shrink(Buffer& buffer) noexcept;

        void resize(Buffer& buffer, u32 new_size) noexcept;

        void clear(Buffer& buffer) noexcept;

        auto data(Buffer& buffer) noexcept -> void*;

        auto append(Buffer& buffer, Data data) noexcept -> void*;

        auto append(Buffer& buffer, const void* data, u32 size, u32 alignment) noexcept -> void*;

    } // namespace buffer

    inline Buffer::operator ice::Data() const noexcept
    {
        return this->operator ice::Memory();
    }

    inline Buffer::operator ice::Memory() const noexcept
    {
        return Memory{
            .location = _data,
            .size = _size,
            .alignment = ice::Allocator::Constant_DefaultAlignment
        };
    }

} // namespace ice
