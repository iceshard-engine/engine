#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in mat4 in_model;

layout (location = 0) out vec4 out_color;

layout (std140, binding = 0) uniform Camera {
    mat4 mvp;
} cam;

void main()
{
    out_color = vec4(in_color, 1);
    gl_Position = cam.mvp * in_model * vec4(in_pos, 1);
}
