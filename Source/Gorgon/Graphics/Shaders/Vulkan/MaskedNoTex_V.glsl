#version 450

#include "Quad.glsl"

layout(location = 1) out vec2 maskcoord;

void main()
{
    gl_Position = quad_position();

    maskcoord = quad_maskcoord();
}
