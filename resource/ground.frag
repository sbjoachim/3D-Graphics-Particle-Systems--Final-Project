// ============================================================
// Ground Plane Fragment Shader
// ============================================================
// Renders the ground with a distance-based fade effect.
// Vertices far from the center gradually fade to the background
// color, creating a natural horizon look instead of hard edges.
// ============================================================

#version 330

// --- Inputs from vertex shader ---
in vec4 vColor;
in vec3 vWorldPos;

// --- Output color ---
out vec4 fragColor;

void main(void)
{
    // Calculate horizontal distance from the world origin
    float dist = length(vWorldPos.xz);

    // Fade factor: goes from 1.0 (at center) to 0.0 (at distance 50+)
    // smoothstep gives a nice gradual transition
    float fade = 1.0 - smoothstep(20.0, 50.0, dist);

    // Apply fade to alpha so the edges blend into the dark background
    fragColor = vec4(vColor.rgb, vColor.a * fade);
}
