#pragma once
#include <ice/mem_info.hxx>
#include <type_traits>

namespace ice
{

    inline auto ptr_add(void* ptr, ice::usize offset) noexcept -> void*
    {
        return reinterpret_cast<char*>(ptr) + offset.value;
    }

    inline auto ptr_add(void const* ptr, ice::usize offset) noexcept -> void const*
    {
        return reinterpret_cast<char const*>(ptr) + offset.value;
    }

    inline auto ptr_distance(void const* ptr_from, void const* ptr_to) noexcept -> ice::isize
    {
        return { reinterpret_cast<char const*>(ptr_to) - reinterpret_cast<char const*>(ptr_from) };
    }

    constexpr auto mem_max_capacity(ice::usize element_size, ice::usize memory_space) noexcept -> ice::ucount
    {
        return static_cast<ice::ucount>(memory_space.value / element_size.value);
    }

    template<typename T>
    constexpr auto mem_max_capacity(ice::usize memory_space) noexcept -> ice::ucount
    {
        return ice::mem_max_capacity(ice::size_of<T>, memory_space);
    }

} // namespace ice
