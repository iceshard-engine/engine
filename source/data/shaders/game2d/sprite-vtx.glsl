#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 2) in vec3 in_offset;
layout (location = 3) in vec2 in_scale;
//layout (location = 3) in float in_angle;
layout (location = 4) in int in_mat_x;
layout (location = 5) in int in_mat_y;

layout (location = 0) out vec2 out_uv;
// layout (location = 1) out int out_mat_x;
// layout (location = 2) out int out_mat_y;

layout (std140, set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
} cam;

void main()
{
    // out_mat_x = in_mat_x;
    // out_mat_y = in_mat_y;
    out_uv = in_uv + vec2(in_mat_x, in_mat_y);

    vec3 pos = vec3(in_pos, 0) + in_offset;
    gl_Position = cam.projection * cam.view * vec4(pos, 1);
}
