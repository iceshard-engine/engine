#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 2) in vec2 in_offset;
layout (location = 3) in int in_matid;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out int out_matid;

layout (std140, set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 clip;
} cam;

void main()
{
    out_matid = in_matid & 0x0000000f;
    int matid_offset_x = (in_matid & 0x0003fff0) >> 4;
    int matid_offset_y = (in_matid & 0xfffc0000) >> 18;

    out_uv = in_uv + vec2(matid_offset_x, mattid_offset_y);

    vec3 pos = vec3(in_pos, 0) + vec3(in_offset, 1);
    gl_Position = cam.clip * cam.projection * cam.view * vec4(pos, 1);
}
