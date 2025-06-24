/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/types_extended.hxx>
#include <ice/build/constants.hxx>
#include <limits>

namespace ice
{

    // Min values
    constexpr ice::f32 const f32_min = std::numeric_limits<ice::f32>::min();
    constexpr ice::f64 const f64_min = std::numeric_limits<ice::f64>::min();
    constexpr ice::i8  const i8_min = std::numeric_limits<ice::i8>::min();
    constexpr ice::i16 const i16_min = std::numeric_limits<ice::i16>::min();
    constexpr ice::i32 const i32_min = std::numeric_limits<ice::i32>::min();
    constexpr ice::i64 const i64_min = std::numeric_limits<ice::i64>::min();
    constexpr ice::u8  const u8_min = std::numeric_limits<ice::u8>::min();
    constexpr ice::u16 const u16_min = std::numeric_limits<ice::u16>::min();
    constexpr ice::u32 const u32_min = std::numeric_limits<ice::u32>::min();
    constexpr ice::u64 const u64_min = std::numeric_limits<ice::u64>::min();

    constexpr ice::ucount const ucount_min = std::numeric_limits<ice::ucount>::min();
    constexpr ice::icount const icount_min = std::numeric_limits<ice::icount>::min();

    // Max values
    constexpr ice::f32 const f32_max = std::numeric_limits<ice::f32>::max();
    constexpr ice::f64 const f64_max = std::numeric_limits<ice::f64>::max();
    constexpr ice::i8  const i8_max = std::numeric_limits<ice::i8>::max();
    constexpr ice::i16 const i16_max = std::numeric_limits<ice::i16>::max();
    constexpr ice::i32 const i32_max = std::numeric_limits<ice::i32>::max();
    constexpr ice::i64 const i64_max = std::numeric_limits<ice::i64>::max();
    constexpr ice::u8  const u8_max = std::numeric_limits<ice::u8>::max();
    constexpr ice::u16 const u16_max = std::numeric_limits<ice::u16>::max();
    constexpr ice::u32 const u32_max = std::numeric_limits<ice::u32>::max();
    constexpr ice::u64 const u64_max = std::numeric_limits<ice::u64>::max();

    constexpr ice::ucount const ucount_max = std::numeric_limits<ice::ucount>::max();
    constexpr ice::icount const icount_max = std::numeric_limits<ice::icount>::max();

    // Special floating point values
    constexpr ice::f32 const f32_inf = std::numeric_limits<ice::f32>::infinity();
    constexpr ice::f64 const f64_inf = std::numeric_limits<ice::f64>::infinity();
    constexpr ice::f32 const f32_nan = std::numeric_limits<ice::f32>::signaling_NaN();
    constexpr ice::f64 const f64_nan = std::numeric_limits<ice::f64>::signaling_NaN();

    // Typed zero values
    constexpr ice::f32 const f32_0 = ice::f32(0.0f);
    constexpr ice::f64 const f64_0 = ice::f64(0.0);
    constexpr ice::i8  const i8_0 = ice::i8(0);
    constexpr ice::i16 const i16_0 = ice::i16(0);
    constexpr ice::i32 const i32_0 = ice::i32(0);
    constexpr ice::i64 const i64_0 = ice::i64(0);
    constexpr ice::u8  const u8_0 = ice::u8(0);
    constexpr ice::u16 const u16_0 = ice::u16(0);
    constexpr ice::u32 const u32_0 = ice::u32(0);
    constexpr ice::u64 const u64_0 = ice::u64(0);

} // namespace ice
