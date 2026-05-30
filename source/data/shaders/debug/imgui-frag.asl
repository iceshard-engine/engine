
import "shaders/debug/imgui"

ctx
{
    #[uniform, group=1, binding=1]
    let smp : Sampler native

    #[uniform, group=1, binding=2]
    let tex2d : Texture2D native
}

fn linear_to_srgb(color : Float) : Float
{
//    linear_to_srgb.xyz = mix(color.xyz / 12.92, pow((color.xyz + 0.055) / 1.055, Vec3f(2.4)), step(0.04045, color.xyz))

    if (color <= 0.0031308f)
    {
        linear_to_srgb = 12.92 * color
    }
    else
    {
        linear_to_srgb = 1.055 * pow(color, 1.0 / 2.4) - 0.055
    }
}

#[shader_main]
#[shader_stage=fragment]
fn main(in : VertexResult) : FragmentOut
{
    // in.color.x = linear_to_srgb(in.color.x)
    // in.color.y = linear_to_srgb(in.color.y)
    // in.color.z = linear_to_srgb(in.color.z)
    main.color = in.color * sampleTexture(smp, tex2d, in.uv)
}
