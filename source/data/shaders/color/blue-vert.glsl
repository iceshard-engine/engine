#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 out_uv;

layout (std140, set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 clip;
} cam;

void main()
{
    out_uv = in_uv;
    gl_Position = cam.clip * vec4(in_pos, 0, 1);
}
