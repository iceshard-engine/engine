/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>

namespace ice::gfx
{

    struct GfxCameraUniform
    {
        ice::mat4x4 view_matrix;
        ice::mat4x4 projection_matrix;
        ice::mat4x4 clip_matrix;
    };

} // namespace ice::gfx
