#version 450

#include "Uniforms.glsl"

layout(location = 1) in vec2 maskcoord;

layout(set = 1, binding = 0) uniform sampler2D mask;

layout(location = 0) out vec4 output_color;

void main()
{
    output_color = vec4(tint.rgb, tint.a * texture(mask, maskcoord).a * texture(mask, maskcoord).r);
}
