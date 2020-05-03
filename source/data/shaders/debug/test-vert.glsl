#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 6) in mat4 in_model;
layout (location = 10) in vec3 in_color;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec3 out_pos;
layout (location = 2) out vec3 out_norm;

layout (std140, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 clip; // unused
} cam;

void main()
{
    out_color = vec4(in_color, 1);
    out_pos = vec3(in_model * vec4(in_pos, 1));
    out_norm = mat3(transpose(inverse(in_model))) * in_norm;
    gl_Position = cam.clip * cam.projection * cam.view * in_model * vec4(in_pos, 1);
}
