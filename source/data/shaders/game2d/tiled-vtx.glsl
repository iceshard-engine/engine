#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 2) in int in_offset;
layout (location = 3) in int in_matid;

layout (location = 0) out vec2 out_uv;
layout (location = 1) out int out_matid;

layout (std140, set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
} cam;

layout (std140, set = 1, binding = 3) uniform TilemapProps
{
    vec2 tile_scale;
    vec2 tile_size;
} tilemap_props[4];

void main()
{
    int offset_x = in_offset & 0x0000ffff;
    int offset_y = (in_offset & 0xffff0000) >> 16;
    vec2 tile_offset = vec2(offset_x, offset_y);

    int uv_offset_x = in_matid & 0x00000fff;
    int uv_offset_y = (in_matid & 0x00fff000) >> 12;
    int uv_matid = (in_matid >> 28) & 0x0000000f;

    out_matid = uv_matid;
    out_uv = in_uv + vec2(uv_offset_x, uv_offset_y);

    vec2 offset = tile_offset * tilemap_props[uv_matid].tile_size;

    vec3 pos = vec3(in_pos + offset, -1);
    gl_Position = cam.projection * cam.view * vec4(pos, 1);
}
