#pragma once
#include "translate.hxx"

namespace core::math
{

    inline auto lookat(vec3f pos, vec3f target, vec3f up) noexcept -> mat4x4
    {
        vec3f const dir = normalize(pos - target);
        vec3f const right = normalize(cross(up, dir));
        vec3f const view_up = cross(dir, right);

        return mat4x4 {
            right.x, view_up.x, dir.x, 0.0f,
            right.y, view_up.y, dir.y, 0.0f,
            right.z, view_up.z, dir.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        } * translate(vec3f{} - pos);
    }

} // namespace core::math
