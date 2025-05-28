#version 450

// declare output
layout(location = 0) out vec3 fragColor;

// hard coded 2d vertex positions
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

// hard coded RGB colours
vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main() {
	// sets position of each vertex
	// gl_VertexIndex is a built-in integer that indicates which vertex (0, 1, 2) is currently being processed.
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	// Passes the per-vertex color to the next stage (fragment shader)
	fragColor = colors[gl_VertexIndex];
}