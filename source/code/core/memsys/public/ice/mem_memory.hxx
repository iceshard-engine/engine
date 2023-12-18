/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_types.hxx>
#include <ice/mem_data.hxx>

namespace ice
{

    struct Memory
    {
        void* location;
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

} // namespace ice
