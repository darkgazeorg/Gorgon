#version 450

#include "Quad.glsl"

layout(location = 0) out vec2 texcoord;
layout(location = 1) out vec2 maskcoord;

void main()
{
    gl_Position = quad_position();

    texcoord = tex_coords[vertex_index];

    maskcoord = quad_maskcoord();
}
