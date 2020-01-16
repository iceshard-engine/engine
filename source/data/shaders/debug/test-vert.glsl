#version 450

layout (std140, binding = 0) uniform bufferVals {
    mat4 vp;
} myBufferVals;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in mat4 inModel;

layout (location = 0) out vec4 outColor;

void main()
{
   outColor = inColor;
   gl_Position = myBufferVals.vp * inModel * pos;
}
