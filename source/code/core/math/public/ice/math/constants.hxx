/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <limits>
namespace ice::math
{

    static constexpr ice::f32 f32_eps = std::numeric_limits<f32>::epsilon();
    static constexpr ice::f32 f32_pi = 3.14159265358979323846f;
    static constexpr ice::f32 f32_half_pi = 3.14159265358979323846f * 0.5f;

    static constexpr ice::f64 f64_eps = std::numeric_limits<f64>::epsilon();
    static constexpr ice::f64 f64_pi = 3.14159265358979323846;
    static constexpr ice::f64 f64_half_pi = 3.14159265358979323846 * 0.5;

} // namespace ice::math
