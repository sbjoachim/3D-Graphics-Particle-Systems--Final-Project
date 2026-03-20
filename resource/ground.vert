// ============================================================
// Ground Plane Vertex Shader
// ============================================================
// Transforms ground plane vertices from world space to screen space.
// Also passes the world-space position to the fragment shader
// so we can calculate distance-based fog/fade effects.
// ============================================================

#version 330

// --- Vertex attributes (must match VBO layout in CreateGroundPlane) ---
layout(location = 0) in vec3 vertex;    // vertex position in world space
layout(location = 1) in vec3 color;     // vertex color
layout(location = 2) in vec3 normal;    // vertex normal (not used for ground, but kept for consistency)

// --- Outputs to fragment shader ---
out vec4 vColor;
out vec3 vWorldPos;

// --- Transformation matrices ---
uniform mat4 projection_mat;
uniform mat4 view_mat;
uniform mat4 world_mat;

void main(void)
{
    // Pass color to fragment shader
    vColor = vec4(color, 1.0);

    // Calculate world position for fog calculations in fragment shader
    vWorldPos = vec3(world_mat * vec4(vertex, 1.0));

    // Standard MVP transformation: model -> world -> camera -> screen
    gl_Position = projection_mat * view_mat * world_mat * vec4(vertex, 1.0);
}
