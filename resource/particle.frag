// ============================================================
// Particle Fragment Shader
// ============================================================
// This shader runs for every pixel of every particle's point sprite.
//
// It does two things:
//   1. Makes each square point look like a soft circle (soft-edge effect)
//   2. Applies the particle's color with transparency (alpha)
//
// gl_PointCoord is a built-in variable that tells us where we are
// within the point sprite:
//   (0,0) = top-left corner
//   (1,1) = bottom-right corner
//   (0.5, 0.5) = center
//
// We use the distance from center to create a circular falloff,
// so particles look like glowing orbs instead of hard squares.
// ============================================================

#version 330

// --- Color received from the vertex shader ---
in vec4 vColor;

// --- Final pixel color output ---
out vec4 fragColor;

void main(void)
{
    // Distance from center of the point sprite (turns squares into circles)
    vec2 center = gl_PointCoord - vec2(0.5);
    float dist = length(center);

    // Discard pixels outside the circle radius
    if (dist > 0.5) {
        discard;
    }

    // Soft glow: fully bright at center, fades at edges
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);

    // Output: full color at center, fades to transparent at edge
    fragColor = vec4(vColor.rgb, glow);
}
