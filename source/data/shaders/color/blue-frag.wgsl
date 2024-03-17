struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

@fragment
fn main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(0.2, 0.2, 0.9, 1.0);
}
