// Basic Vertex Shader
// Minimal shader for testing the foundation setup

#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

out vec4 vColor;

uniform mat4 projection_mat;
uniform mat4 view_mat;
uniform mat4 world_mat;

void main(void)
{
	vColor = vec4(color, 1.0);
	gl_Position = projection_mat * view_mat * world_mat * vec4(vertex, 1.0);
}
