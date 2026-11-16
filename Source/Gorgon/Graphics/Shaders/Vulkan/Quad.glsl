#include "Uniforms.glsl"

// Two triangles that form a quad. There are no vertex buffers, vertices are selected by their index.
const int quad_indices[6] = int[6](0, 3, 1, 1, 3, 2);

int vertex_index = quad_indices[gl_VertexIndex];

// Vertex coordinates are in OpenGL clip space, where z is between -1 and 1. Vulkan uses 0 to 1.
vec4 quad_position() {
    vec3 vertex = vertex_coords[vertex_index].xyz;

    return vec4(vertex.xy, (vertex.z + 1.0) / 2.0, 1.0);
}

vec2 quad_maskcoord() {
    return (vertex_coords[vertex_index].xy + 1.0) / 2.0;
}
