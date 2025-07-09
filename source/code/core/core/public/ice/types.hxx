/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <cmath>
#include <cstdint>

namespace ice
{

    using utf8 = char8_t; // might need to be changed to unisgned char later.
    using utf16 = char16_t;
    using utf32 = char32_t;
    using wchar = wchar_t;

    using f32 = float;
    using f64 = double;

    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using uptr = std::uintptr_t;

    // Declaration of ref types

    //! \brief Holds 'offset' and 'size' fields (u32) to access data stored in a buffer-like object.
    using ref32 = struct { ice::u32 offset; ice::u32 size; };

    //! \brief Holds 'offset' and 'size' fields (u16) to access data stored in a buffer-like object.
    struct ref16
    {
        ice::u16 offset;
        ice::u16 size;

        constexpr operator ice::ref32() const noexcept { return ref32{ offset, size }; }
    };

    //! \brief Holds 'offset' and 'size' fields (u8) to access data stored in a buffer-like object.
    struct ref8
    {
        ice::u8 offset;
        ice::u8 size;

        constexpr operator ice::ref16() const noexcept { return ref16{ offset, size }; }
        constexpr operator ice::ref32() const noexcept { return ref32{ offset, size }; }
    };

    // Forward declaration of time-types
    struct Ts;
    struct Tms;
    struct Tus;
    struct Tns;

    // Note: Added these checks, because the previous definition using std::float_t resulted in f32 being a 'long double' type.
    static_assert(sizeof(f32) == 4, "We expect 'float' types to be 4 bytes.");
    static_assert(sizeof(f64) == 8, "We expect 'double' types to be 8 bytes.");

} // namespace ice
