#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_uv;
// layout (location = 3) in mat4 in_model;

layout (location = 0) out vec2 out_uv;

// layout (std140, set = 1, binding = 2) uniform Camera
// {
//     mat4 view;
//     mat4 projection;
//     mat4 clip;
// } cam;

void main()
{
    gl_Position = vec4(in_pos, 1);
    out_uv = in_uv;
    // gl_Position =  cam.clip * cam.projection * cam.view * /*in_model **/ vec4(in_pos, 1);
}
