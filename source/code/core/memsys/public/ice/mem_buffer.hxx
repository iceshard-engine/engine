#pragma once
#include <ice/mem_memory.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/mem_data.hxx>

namespace ice
{

    struct Buffer
    {
        ice::Allocator* alloc;
        ice::Memory memory{ };
        ice::meminfo used{ .size = 0_B, .alignment = ice::ualign::b_default };
    };

    namespace buffer
    {

        inline void set_capacity(ice::Buffer& buffer, ice::usize new_capacity) noexcept;
        inline void grow(ice::Buffer& buffer, ice::usize min_capacity) noexcept;
        inline auto append_reserve(ice::Buffer& buffer, ice::meminfo meminfo) noexcept -> ice::Memory;

        inline auto size(ice::Buffer const& buffer) noexcept -> ice::usize;
        inline auto capacity(ice::Buffer const& buffer) noexcept -> ice::usize;
        inline auto required_capacity(ice::Buffer const& buffer, ice::meminfo meminfo) noexcept -> ice::usize;
        inline auto space(ice::Buffer const& buffer) noexcept -> ice::usize;
        inline bool empty(ice::Buffer const& buffer) noexcept;
        inline bool has_space(ice::Buffer const& buffer, ice::meminfo meminfo) noexcept;

    } // namespace buffer

    namespace buffer
    {

        inline void set_capacity(ice::Buffer& buffer, ice::usize new_capacity) noexcept
        {
            if (buffer.memory.size == new_capacity)
            {
                return;
            }

            ice::Memory new_data{ };
            if (new_capacity > 0_B)
            {
                new_data = buffer.alloc->allocate({ new_capacity, buffer.memory.alignment });
                ice::memcpy(new_data, ice::data_view(buffer.memory));
            }

            buffer.alloc->deallocate(buffer.memory);
            buffer.memory = new_data;
            buffer.used.size = ice::min(new_capacity, buffer.used.size);
        }

        inline void grow(ice::Buffer& buffer, ice::usize min_capacity) noexcept
        {
            ice::usize const new_size = buffer.memory.size * 2 + 256_B;
            set_capacity(buffer, ice::max(min_capacity, new_size));
        }

        inline auto append_reserve(ice::Buffer& buffer, ice::meminfo meminfo) noexcept -> ice::Memory
        {
            ice::usize const req_capacity = ice::buffer::required_capacity(buffer, meminfo);
            if (req_capacity == ice::buffer::capacity(buffer))
            {
                ice::buffer::grow(buffer, req_capacity);
            }

            ice::usize const offset = buffer.used += meminfo;
            return Memory{
                .location = ice::ptr_add(buffer.memory.location, offset),
                .size = meminfo.size,
                .alignment = meminfo.alignment
            };
        }


        inline auto size(ice::Buffer const& buffer) noexcept -> ice::usize
        {
            return buffer.used.size;
        }

        inline auto capacity(ice::Buffer const& buffer) noexcept -> ice::usize
        {
            return buffer.memory.size;
        }

        inline auto required_capacity(ice::Buffer const& buffer, ice::meminfo meminfo) noexcept -> ice::usize
        {
            ice::meminfo tempinfo = buffer.used;
            tempinfo += meminfo;
            return tempinfo.size;
        }

        inline auto space(ice::Buffer const& buffer) noexcept -> ice::usize
        {
            return { buffer.memory.size.value - buffer.used.size.value };
        }

        inline bool empty(ice::Buffer const& buffer) noexcept
        {
            return buffer.used.size == 0_B;
        }

        inline bool has_space(ice::Buffer const& buffer, ice::meminfo meminfo) noexcept
        {
            return ice::buffer::capacity(buffer) >= ice::buffer::required_capacity(buffer, meminfo);
        }

        inline auto memory_pointer(ice::Buffer const& buffer) noexcept
        {
            return buffer.memory.location;
        }

    } // namespace buffer

} // namespace ice
