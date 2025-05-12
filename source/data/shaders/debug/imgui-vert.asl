
import "shaders/debug/imgui"

struct Camera
{
    vp : Mat4x4f
}

ctx
{
    #[uniform, group = 0, binding = 3]
    let cam : Camera native
}

#[shader_main]
#[shader_stage=vertex]
fn main(in : VertexIn) : VertexResult
{
    main.uv = in.uv
    main.color = in.color
    main.pos = cam.vp * Vec4f(in.pos, 0.0, 1.0)
}
