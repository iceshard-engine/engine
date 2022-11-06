/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_types.hxx>

namespace ice
{

    struct Memory
    {
        void* location;
        ice::usize size;
        ice::ualign alignment;
    };

} // namespace ice
