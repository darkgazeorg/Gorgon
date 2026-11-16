#version 450

#include "Uniforms.glsl"

layout(location = 0) out vec4 output_color;

void main()
{
    output_color = vec4(tint.a, 0, 0, 1);
}
