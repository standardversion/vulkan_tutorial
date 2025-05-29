#version 450
// declare input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
// declare output
layout(location = 0) out vec3 fragColor;

void main() {
	// sets position of each vertex
	gl_Position = vec4(inPosition, 0.0, 1.0);
	// Passes the per-vertex color to the next stage (fragment shader)
	fragColor = inColor;
}