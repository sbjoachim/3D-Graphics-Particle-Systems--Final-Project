// ============================================================
// Particle Vertex Shader
// ============================================================
// This shader positions each particle in 3D space and passes
// its color and size to the fragment shader.
//
// Each particle is rendered as a single GL_POINT (a square dot
// on screen). The size of that dot is controlled by gl_PointSize.
//
// Inputs (from CPU via vertex attributes):
//   position  - where the particle is in world space (x, y, z)
//   color     - RGBA color of the particle (includes transparency)
//   psize     - how big the point should appear on screen
//
// Uniforms (set once per frame from C++ code):
//   view_mat       - camera's view matrix (world -> camera space)
//   projection_mat - perspective projection matrix (camera -> screen)
// ============================================================

#version 330

// --- Vertex attributes (per-particle data from the VBO) ---
layout(location = 0) in vec3 position;   // particle world position
layout(location = 1) in vec4 color;      // particle RGBA color
layout(location = 2) in float psize;     // particle point size

// --- Data passed to the fragment shader ---
out vec4 vColor;

// --- Transformation matrices (set from C++ each frame) ---
uniform mat4 view_mat;
uniform mat4 projection_mat;

void main(void)
{
    // Pass the color through to the fragment shader unchanged
    vColor = color;

    // Transform the particle position:
    //   1. view_mat moves it from world space to camera space
    //   2. projection_mat applies perspective (things farther away look smaller)
    vec4 viewPos = view_mat * vec4(position, 1.0);
    gl_Position = projection_mat * viewPos;

    // Set the size of the point sprite on screen.
    // We scale by distance so particles look smaller when far away.
    // viewPos.z is negative (OpenGL camera looks down -Z), so we use length.
    // Clamp the result to avoid exceeding the GPU's max point size.
    float dist = length(viewPos.xyz);
    gl_PointSize = clamp(psize * (80.0 / max(dist, 1.0)), 2.0, 128.0);
}
