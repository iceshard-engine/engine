/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <ice/mem_types.hxx>

namespace ice
{

    struct Data
    {
        void const* location;
        ice::usize size;
        ice::ualign alignment;
    };

    constexpr auto data_view(Memory memory) noexcept
    {
        return Data{
            .location = memory.location,
            .size = memory.size,
            .alignment = memory.alignment
        };
    }

    template<typename Type>
    inline auto data_view(Type const& var) noexcept
    {
        return Data{
            .location = std::addressof(var),
            .size = ice::size_of<Type>,
            .alignment = ice::align_of<Type>
        };
    }

    template<typename Type, ice::usize::base_type Size>
    constexpr auto data_view(Type const(&var)[Size]) noexcept
    {
        return Data{
            .location = std::addressof(var),
            .size = ice::size_of<Type> *Size,
            .alignment = ice::align_of<Type>
        };
    }

} // namespace ice
