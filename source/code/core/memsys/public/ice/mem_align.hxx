/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_types.hxx>
#include <type_traits>

namespace ice
{

    template<typename T> requires std::is_pointer_v<T> || std::is_same_v<T, ice::usize> || std::is_integral_v<T>
    struct AlignResult
    {
        T value;
        T padding;
        ice::ualign alignment;
    };

    template<typename T> requires std::is_pointer_v<T*>
    struct AlignResult<T*>
    {
        T* value;
        ice::usize padding;
        ice::ualign alignment;
    };

    inline bool is_aligned(void const* ptr, ice::ualign alignment) noexcept
    {
        ice::u32 const align_mask = (static_cast<std::underlying_type_t<ice::ualign>>(alignment) - 1);
        ice::uptr const ptr_value = reinterpret_cast<ice::uptr>(ptr);
        return (ptr_value & align_mask) == 0;
    }

    inline bool is_aligned(ice::u32 val, ice::ualign alignment) noexcept
    {
        ice::u32 const align_mask = (static_cast<std::underlying_type_t<ice::ualign>>(alignment) - 1);
        return (val & align_mask) == 0;
    }

    template<typename T> requires std::is_pointer_v<T> || std::is_same_v<T, ice::usize> || std::is_integral_v<T>
    constexpr auto align_to(T value, ice::ualign alignment) noexcept -> ice::AlignResult<T>
    {
        ICE_ASSERT_CORE(alignment != ice::ualign::invalid);
        ice::AlignResult<T> result{
            .alignment = alignment
        };

        ice::u32 const align_mask = (static_cast<std::underlying_type_t<ice::ualign>>(alignment) - 1);
        if constexpr (std::is_pointer_v<T>)
        {
            ice::uptr const ptr_value = reinterpret_cast<ice::uptr>(value);
            result.padding = { (ice::uptr{0} - ptr_value) & align_mask };
            result.value = reinterpret_cast<T>(ptr_value + result.padding.value);
        }
        else if constexpr (std::is_same_v<T, ice::usize>)
        {
            result.padding = { ((typename T::base_type{0}) - (value.value)) & align_mask };
            result.value = value + result.padding;
        }
        else
        {
            result.padding = (T{ 0 } - value) & align_mask;
            result.value = value + result.padding;
        }
        return result;
    }

} // namespace ice
