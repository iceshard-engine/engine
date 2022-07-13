#pragma once
#include <ice/mem_types.hxx>

namespace ice
{

    template<typename T> requires std::is_pointer_v<T> || std::is_same_v<T, ice::usize>
    struct align_result
    {
        T value;
        ice::usize padding;
        ice::ualign alignment;
    };

    template<typename T> requires std::is_pointer_v<T> || std::is_same_v<T, ice::usize>
    constexpr auto align_to(T value, ice::ualign alignment) noexcept -> ice::align_result<T>
    {
        ice::align_result<T> result{
            .value = value,
            .alignment = alignment
        };

        ice::u32 const align_mask = (static_cast<std::underlying_type_t<ice::ualign>>(alignment) - 1);
        if constexpr (std::is_pointer_v<T>)
        {
            ice::uptr const ptr_value = reinterpret_cast<ice::uptr>(result.value);
            result.padding = T{ (ice::uptr{0} - ptr_value) & align_mask };
            result.value = reinterpret_cast<T>(ptr_value + result.padding.value);
        }
        else
        {
            result.padding = { ((typename T::base_type{ 0 }) - (result.value.value)) & align_mask};
            result.value += result.padding;
        }
        return result;
    }

} // namespace ice
