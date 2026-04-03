// Vertex shader for fire pit geometry (stones, logs, charcoal)
// Passes world-space position and normal to fragment shader for lighting

#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

out vec3 fWorldPos;
out vec3 fNormal;
out vec3 fColor;

uniform mat4 projection_mat;
uniform mat4 view_mat;
uniform mat4 world_mat;

void main(void)
{
    fWorldPos = (world_mat * vec4(vertex, 1.0)).xyz;
    fNormal = normalize(mat3(world_mat) * normal);
    fColor = color;
    gl_Position = projection_mat * view_mat * world_mat * vec4(vertex, 1.0);
}
