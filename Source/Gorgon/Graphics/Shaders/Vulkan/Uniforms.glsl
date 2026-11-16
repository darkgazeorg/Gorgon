// Uniforms of a quad. This should match GL::Uniforms in GL/Vulkan.cpp. Vulkan guarantees at least
// 128 bytes of push constants, and a quad needs 112.
layout(push_constant) uniform Uniforms {
    vec4 vertex_coords[4];
    vec2 tex_coords[4];
    vec4 tint;
};
