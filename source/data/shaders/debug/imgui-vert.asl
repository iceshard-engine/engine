
import "shaders/debug/imgui"

#[shader_stage=vertex]
fn oklchu8_to_oklch(color : UInt) : Vec4f
{
    let u8light : UInt = (color & 0x000000ff)
    let u8chroma : UInt = (color & 0x0000ff00) >> 8
    let u8hue : UInt = (color & 0x00ff0000) >> 16
    let alpha : UInt = (color & 0xff000000) >> 24

    let light : Float = (u8light & 0x7f) * 0.01
    let hue : Float = u8hue + ((u8light & 0x80) << 1)

    oklchu8_to_oklch.x = light
    oklchu8_to_oklch.y = u8chroma / 510.0
    oklchu8_to_oklch.z = hue
    oklchu8_to_oklch.w = alpha / 255.0
}

#[shader_stage=vertex]
fn oklch_to_oklab(color : Vec4f) : Vec4f
{
    oklch_to_oklab.x = color.x
    oklch_to_oklab.y = color.y * cos(radians(color.z))
    oklch_to_oklab.z = color.y * sin(radians(color.z))
    oklch_to_oklab.w = color.w
}

#[shader_stage=vertex]
fn oklab_to_lrgb(color : Vec4f) : Vec4f
{
    let l2 : Float = color.x + color.y * 0.3963377774 + color.z * 0.2158037573
    let m2 : Float = color.x + color.y * -0.1055613458 + color.z * -0.0638541728
    let s2 : Float = color.x + color.y * -0.0894841775 + color.z * -1.291485548

    // Cube
    let l3 : Float = l2 * l2 * l2
    let m3 : Float = m2 * m2 * m2
    let s3 : Float = s2 * s2 * s2

    oklab_to_lrgb.x = (4.0767416621 * l3) - (3.3077115913 * m3) + (0.2309699292 * s3)
    oklab_to_lrgb.y = (-1.2684380046 * l3) + (2.6097574011 * m3) - (0.3413193965 * s3)
    oklab_to_lrgb.z = (-0.0041960863 * l3) - (0.7034186147 * m3) + (1.7076147010 * s3)
    oklab_to_lrgb.w = color.w
}

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
    //main.color = oklab_to_lrgb(oklch_to_oklab(Vec4f(0.65, 0.12, 234, 1.0))) // in.color
    //main.color = oklab_to_lrgb(oklch_to_oklab(oklchu8_to_oklch(in.color)))
    main.color = in.color
    main.pos = cam.vp * Vec4f(in.pos, 0.0, 1.0)
}
