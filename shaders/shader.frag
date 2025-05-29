#version 450

layout(binding = 1) uniform sampler2D texSampler;
// Declares an input variable from the vertex shader:
// Matches the location = 0 output from the vertex shader.
// will automatically interpolate the colours between the verts
// layout(location = 0) flat in vec3 fragColor; // disables interpolation
// layout(location = 0) noperspective in vec3 fragColor; // disables perspective correction
// layout(location = 0) smooth in vec3 fragColor; // default; uses perspective-correct interpolation

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

//The final RGBA color to write to the framebuffer
layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(texSampler, fragTexCoord);
}