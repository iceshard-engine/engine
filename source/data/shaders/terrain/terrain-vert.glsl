#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_uv;

layout (location = 6) in vec2 in_model;
layout (location = 7) in uvec2 in_tile;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out vec3 out_pos;
layout (location = 2) out vec3 out_norm;

layout (std140, set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 clip;
} cam;

vec2 tile_uv = vec2(16.0 / 368.0, 16.0 / 224.0);

void main()
{
    out_uv = (vec2(in_tile) + in_uv) * tile_uv;
    out_pos = vec3(in_model, 0) + in_pos;
    out_norm = in_norm;
    gl_Position = cam.clip * cam.projection * cam.view * vec4(out_pos, 1);
}
