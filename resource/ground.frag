// ============================================================
// Ground Plane Fragment Shader
// ============================================================
// Renders a dark, atmospheric ground with:
//   - Subtle grid lines that catch light near the center
//   - Warm emitter glow pooling around the origin
//   - Smooth distance fade to the dark sky at edges
// ============================================================

#version 330

// --- Inputs from vertex shader ---
in vec4 vColor;
in vec3 vWorldPos;

// --- Output color ---
out vec4 fragColor;

void main(void)
{
    float dist = length(vWorldPos.xz);

    // === Subtle grid pattern ===
    // Fine grid lines that shimmer near the center
    vec2 grid = abs(fract(vWorldPos.xz * 0.5) - 0.5);
    float line = min(grid.x, grid.y);
    float gridMask = 1.0 - smoothstep(0.0, 0.04, line);  // thin lines
    // Grid only visible near center, fades with distance
    float gridFade = 1.0 - smoothstep(5.0, 25.0, dist);
    float gridBright = gridMask * gridFade * 0.12;

    // === Warm emitter glow on the ground ===
    // Simulates light pooling from fire/particles above
    float glowRadius = 8.0;
    float glow = exp(-dist * dist / (glowRadius * glowRadius));
    vec3 warmGlow = vec3(0.18, 0.08, 0.03) * glow;   // warm amber
    vec3 hotSpot  = vec3(0.25, 0.12, 0.04) * exp(-dist * dist / 4.0); // tighter bright center

    // === Base ground color with depth ===
    // Radial gradient: slightly lighter near center, darker at edges
    float radialShade = 0.04 + 0.03 * (1.0 - smoothstep(0.0, 30.0, dist));
    vec3 baseColor = vColor.rgb + vec3(radialShade);

    // === Combine layers ===
    vec3 col = baseColor + warmGlow + hotSpot + vec3(gridBright * 0.6, gridBright * 0.7, gridBright);

    // === Distance fade to background ===
    float fade = 1.0 - smoothstep(20.0, 50.0, dist);

    fragColor = vec4(col, fade);
}
