#version 450

layout (location = 0) in vec2 in_uv[];
layout (std140, set = 1, binding = 2) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 clip;
} cam;

layout(std140, set = 1, binding = 3) uniform TesSettings
{
    float level_inner;
    float level_outer;
} tess_settings;

layout(set = 1, binding = 0) uniform texture2D hm_image;
layout(set = 1, binding = 1) uniform sampler hm_sampler;

layout(vertices = 4) out;
layout(location = 0) out vec2 tes_uv[];

void main()
{
    if ( 0 == gl_InvocationID )
    {
        float dist[3];
        float factors[3];

        for ( int i = 0; i < 3; ++i )
        {
            float height = texture(sampler2D(hm_image, hm_sampler), in_uv[i]).x;
            vec4 position = cam.clip * cam.view * (gl_in[i].gl_Position + vec4(0.0, height, 0.0, 0.0));
            dist[i] = dot(position, position);
        }

        factors[0] = min( dist[1], dist[2] );
        factors[1] = min( dist[2], dist[0] );
        factors[2] = min( dist[0], dist[1] );

        gl_TessLevelInner[0] = tess_settings.level_inner; // max( 1.0, 20.0 - factors[0] );
        gl_TessLevelInner[1] = tess_settings.level_inner; // max( 1.0, 20.0 - factors[0] );
        gl_TessLevelOuter[0] = tess_settings.level_outer; // max( 1.0, 20.0 - factors[0] );
        gl_TessLevelOuter[1] = tess_settings.level_outer; // max( 1.0, 20.0 - factors[0] );
        gl_TessLevelOuter[2] = tess_settings.level_outer; // max( 1.0, 20.0 - factors[0] );
        gl_TessLevelOuter[3] = tess_settings.level_outer; // max( 1.0, 20.0 - factors[0] );

        // gl_TessLevelInner[0] = tess_settings.level_inner; // max( 1.0, 20.0 - factors[0] );
        // gl_TessLevelOuter[0] = tess_settings.level_outer; // max( 1.0, 20.0 - factors[0] );
        // gl_TessLevelOuter[1] = 256; // max( 1.0, 20.0 - factors[1] );
        // gl_TessLevelOuter[2] = 256; // max( 1.0, 20.0 - factors[2] );
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tes_uv[gl_InvocationID] = in_uv[gl_InvocationID];
}
