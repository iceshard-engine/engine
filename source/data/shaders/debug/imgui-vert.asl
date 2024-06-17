import shaders/debug/imgui

struct PushConstant
{
    scale : Vec2f
    translate : Vec2f
}

ctx
{
    #[push_constant]
    let pc : PushConstant native
}

#[shader_main]
#[shader_stage=vertex]
fn main(in : VertexIn) : VertexResult
{
    main.uv = in.uv
    main.color = in.color
    main.pos = Vec4f(in.pos * pc.scale + pc.translate, 0.0, 1.0)
}
