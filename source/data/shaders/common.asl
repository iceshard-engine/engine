
#[glsl:type="vec2"]
#[wgsl:type="vec2<f32>"]
def Vec1f native

#[glsl:type="vec2"]
#[wgsl:type="vec2<f32>"]
def Vec2f native

#[glsl:type="vec3"]
#[wgsl:type="vec3<f32>"]
def Vec3f native

#[glsl:type="vec4"]
#[wgsl:type="vec4<f32>"]
def Vec4f native

#[glsl:type="mat4"]
#[wgsl:type="mat4x4<f32>"]
def Mat4x4f native

#[glsl:type="sampler"]
#[wgsl:type="sampler"]
def Sampler native

#[glsl:type="texture2D"]
#[wgsl:type="texture_2d<f32>"]
def Texture2D native

// Defines a texture sampling function
fn sampleTexture(
    sampler : Sampler,
    texture : Texture2D,
    cords : Vec2f
) : Vec4f native
