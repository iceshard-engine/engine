import "shaders/debug/imgui"

ctx
{
    #[uniform, group=1, binding=1]
    let smp : Sampler native

    #[uniform, group=1, binding=2]
    let tex2d : Texture2D native
}

#[shader_main]
#[shader_stage=fragment]
fn main(in : VertexResult) : FragmentOut
{
    main.color = in.color * sampleTexture(smp, tex2d, in.uv)
}
