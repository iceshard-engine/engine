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
} cam;

layout (std140, set = 1, binding = 3) uniform TilemapProps
{
    vec2 tile_scale;
    vec2 tile_size;
} tilemap_props[4];

void main()
{
    int matid =  in_matid & 0x0000000f;
    int uv_offset_x = (in_matid & 0x0003fff0) >> 4;
    int uv_offset_y = (in_matid & 0xfffc0000) >> 18;

    out_matid = matid;
    out_uv = in_uv + vec2(uv_offset_x, uv_offset_y);

    vec2 offset = in_offset * tilemap_props[matid].tile_size;

    vec3 pos = vec3(in_pos + offset, -1);// + vec3(in_offset, -1);
    gl_Position = cam.projection * cam.view * vec4(pos, 1);
}
