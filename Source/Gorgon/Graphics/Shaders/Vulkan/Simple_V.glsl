#version 450

#include "Quad.glsl"

layout(location = 0) out vec2 texcoord;

void main()
{
    gl_Position = quad_position();

    texcoord = tex_coords[vertex_index];
}
