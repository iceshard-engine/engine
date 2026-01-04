/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>

namespace ice
{

    //! \brief Holds 'offset' and 'size' fields (u32) to access data stored in a buffer-like object.
    struct ref32
    {
        ice::u32 offset;
        ice::u32 size;
    };

    //! \brief Holds 'offset' and 'size' fields (u16) to access data stored in a buffer-like object.
    struct ref16
    {
        ice::u16 offset;
        ice::u16 size;

        constexpr operator ice::ref32() const noexcept { return { offset, size }; }
    };

    //! \brief Holds 'offset' and 'size' fields (u8) to access data stored in a buffer-like object.
    struct ref8
    {
        ice::u8 offset;
        ice::u8 size;

        constexpr operator ice::ref16() const noexcept { return { offset, size }; }
        constexpr operator ice::ref32() const noexcept { return { offset, size }; }
    };

} // namespace ice
