#version 450

#include "Quad.glsl"

void main()
{
    gl_Position = quad_position();
}
