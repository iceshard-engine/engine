#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 2) in vec3 in_offset;
layout (location = 3) in float in_angle;

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

//    float rx = in_pos.x * cos(in_angle) - in_pos.y * sin(in_angle);
//    float ry = in_pos.x * sin(in_angle) + in_pos.y * cos(in_angle);
//    vec3 pos = vec3(rx, ry, 0) + in_offset;
    vec3 pos = vec3(in_pos, 0) + in_offset;

    gl_Position = cam.clip * cam.projection * cam.view * vec4(pos, 1);
}
