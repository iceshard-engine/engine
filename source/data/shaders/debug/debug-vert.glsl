#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_color;

layout (std140, set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
} cam;

void main()
{
    out_color = in_color;

    vec4 pos = cam.projection * cam.view * vec4(in_pos, 1);
    gl_Position = vec4(pos.xy, 0, 1);
}
