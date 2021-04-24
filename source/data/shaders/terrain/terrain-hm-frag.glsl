#version 450

layout (location = 0) in vec3 in_norm;
layout (location = 1) in float in_height;
layout (location = 0) out vec4 out_color;

void main()
{
    const vec4 green = vec4( 0.2, 0.5, 0.1, 1.0 );
    const vec4 brown = vec4( 0.6, 0.5, 0.3, 1.0 );
    const vec4 white = vec4( 1.0 );

    vec4 color = mix( green, brown, smoothstep( 0.0, 0.4, in_height ) );
    color = mix( color, white, smoothstep( 0.6, 0.9, in_height ) );

    float diffuse_light = max( 0.0, dot( in_norm, vec3(0.58) ) );
    out_color = vec4( 0.05, 0.05, 0.0, 0.0 ) + diffuse_light * color;
    out_color.w = 1.0;
}
