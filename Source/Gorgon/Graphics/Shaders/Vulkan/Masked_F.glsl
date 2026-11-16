#version 450

#include "Uniforms.glsl"

layout(location = 0) in vec2 texcoord;
layout(location = 1) in vec2 maskcoord;

layout(set = 0, binding = 0) uniform sampler2D diffuse;
layout(set = 1, binding = 0) uniform sampler2D mask;

layout(location = 0) out vec4 output_color;

void main()
{
    output_color = texture(diffuse, texcoord) * vec4(tint.rgb, tint.a * texture(mask, maskcoord).a * texture(mask, maskcoord).r);
}
