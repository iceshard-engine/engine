#version 450

layout( triangles ) in;
layout( location = 0 ) in float in_tess_height[];

layout (std140, set = 1, binding = 2) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 clip;
} cam;

layout(triangle_strip, max_vertices = 3) out;
layout(location = 0) out vec3 out_geom_norm;
layout(location = 1) out float out_geom_height;

void main()
{
    vec3 v0v1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 v0v2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 norm = normalize(cross(v0v1, v0v2));

    for( int vertex = 0; vertex < 3; ++vertex )
    {
        gl_Position = cam.clip * cam.projection * cam.view * gl_in[vertex].gl_Position;
        out_geom_height = in_tess_height[vertex];
        out_geom_norm = norm;
        EmitVertex();
    }

    EndPrimitive();
}
