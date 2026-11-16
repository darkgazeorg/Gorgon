#version 450

#include "Uniforms.glsl"

layout(location = 0) in vec2 texcoord;

layout(set = 0, binding = 0) uniform sampler2D diffuse;

layout(location = 0) out vec4 output_color;

void main()
{
    output_color = vec4(tint.rgb, texture(diffuse, texcoord).a * tint.a);
}
