import "shaders/common"

#[shader_stage=vertex]
struct VertexIn
{
    #[location=0]
    pos : Vec2f

    #[location=1]
    uv : Vec2f

    #[location=2]
    color : Vec4f
}

struct VertexResult
{
    #[shader_stage=vertex]
    #[builtin=position]
    pos : Vec4f

    #[location=0]
    uv : Vec2f

    #[location=1]
    color : Vec4f
}

#[shader_stage=fragment]
struct FragmentOut
{
    #[location=0]
    color : Vec4f
}
