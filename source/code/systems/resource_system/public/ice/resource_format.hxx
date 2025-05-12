/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    static constexpr ice::u32 Constant_ResourceFormatMagic = 'ISRF';
    static constexpr ice::u32 Constant_ResourceFormatVersion = 'V001';

    struct ResourceFormatHeader
    {
        ice::u32 magic;
        ice::u32 version;

        ice::u32 name_size;
        ice::u32 meta_offset;
        ice::u32 meta_size;
        ice::u32 offset;
        ice::u32 size;
    };

} // namespace ice
