
float4x4 WorldViewProjection;
float4 VertexShaderFunction(float4 inputPosition : POSITION) : POSITION
{
    return mul(inputPosition, WorldViewProjection);
}
