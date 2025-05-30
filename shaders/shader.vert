#version 450

//The binding directive is similar to the location directive for attributes. We’re going
//to reference this binding in the descriptor layout.
layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

// declare input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
// declare output
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	// sets position of each vertex
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	// Passes the per-vertex color to the next stage (fragment shader)
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}