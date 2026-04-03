// ============================================================
// Particle Fragment Shader
// ============================================================
// Renders each particle with effect-specific visual treatments:
//   Fire:      Hot white core fading to orange/red edges
//   Smoke:     Soft gaussian puffs with volumetric density
//   Fireworks: Sharp bright core with 6-pointed sparkle rays
//
// gl_PointCoord gives us (0,0)-(1,1) across the point sprite.
// effectType uniform selects the visual treatment.
// ============================================================

#version 330

// --- Color received from the vertex shader ---
in vec4 vColor;

// --- Final pixel color output ---
out vec4 fragColor;

// --- Effect type: 0=fire, 1=smoke, 2=fireworks ---
uniform int effectType;

void main(void)
{
    vec2 center = gl_PointCoord - vec2(0.5);
    float dist = length(center);

    // Discard pixels outside the circle radius
    if (dist > 0.5) {
        discard;
    }

    // Normalized distance (0 at center, 1 at edge)
    float n = dist * 2.0;

    // ---- FIRE: Layered heat gradient ----
    if (effectType == 0) {
        // Three-layer falloff simulates temperature zones
        float core = exp(-12.0 * n * n);                     // white-hot inner core
        float mid  = 1.0 - smoothstep(0.0, 0.6, n);         // orange mid-zone
        float outer = 1.0 - smoothstep(0.3, 1.0, n);        // red outer fringe

        // Blend white-hot core into the particle's natural color
        vec3 hotWhite = vec3(1.0, 0.95, 0.85);
        vec3 col = mix(vColor.rgb, hotWhite, core * 0.7);

        // Boost brightness where layers overlap
        float intensity = outer * (0.5 + 0.5 * mid);
        fragColor = vec4(col, vColor.a * intensity);
    }

    // ---- SMOKE: Volumetric gaussian puff ----
    else if (effectType == 1) {
        // Gaussian falloff gives soft, puffy edges
        float gauss = exp(-4.0 * n * n);

        // Secondary sharper layer adds density in the center
        float dense = 1.0 - smoothstep(0.0, 0.7, n);
        float shape = mix(gauss, dense, 0.4);

        // Subtle darkening at the very center for depth
        vec3 col = vColor.rgb * (0.85 + 0.15 * (1.0 - gauss));

        fragColor = vec4(col, vColor.a * shape);
    }

    // ---- FIREWORKS: Sharp core + sparkle rays ----
    else {
        // Very tight, bright core
        float core = exp(-18.0 * n * n);

        // Soft outer glow
        float glow = 1.0 - smoothstep(0.0, 1.0, n);

        // 6-pointed star ray pattern from the center
        float angle = atan(center.y, center.x);
        float rays = 0.5 + 0.5 * cos(angle * 6.0);
        float sparkle = rays * (1.0 - smoothstep(0.0, 0.7, n)) * 0.25;

        float brightness = clamp(core + glow * 0.4 + sparkle, 0.0, 1.0);

        // White-hot center blending into the particle color
        vec3 col = mix(vColor.rgb, vec3(1.0), core * 0.65);

        fragColor = vec4(col, vColor.a * brightness);
    }
}
