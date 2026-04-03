// Fragment shader for fire pit geometry
// Applies procedural wood texture to logs and lit shading to stones

#version 330

in vec3 fWorldPos;
in vec3 fNormal;
in vec3 fColor;

out vec4 fragColor;

// Hash functions for procedural noise generation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float hash3(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

// 2D value noise with smoothstep interpolation
float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// 3D value noise - trilinear interpolation of hashed lattice points
float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash3(i);
    float n100 = hash3(i + vec3(1,0,0));
    float n010 = hash3(i + vec3(0,1,0));
    float n110 = hash3(i + vec3(1,1,0));
    float n001 = hash3(i + vec3(0,0,1));
    float n101 = hash3(i + vec3(1,0,1));
    float n011 = hash3(i + vec3(0,1,1));
    float n111 = hash3(i + vec3(1,1,1));
    float x00 = mix(n000, n100, f.x);
    float x10 = mix(n010, n110, f.x);
    float x01 = mix(n001, n101, f.x);
    float x11 = mix(n011, n111, f.x);
    float y0 = mix(x00, x10, f.y);
    float y1 = mix(x01, x11, f.y);
    return mix(y0, y1, f.z);
}

// Fractal Brownian Motion - layered noise for natural-looking variation
float fbm3(vec3 p) {
    float val = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; i++) {
        val += amp * noise3D(p);
        p *= 2.1;
        amp *= 0.45;
    }
    return val;
}

// Generates a wood color from world position using ring patterns and grain
vec3 woodTexture(vec3 pos) {
    // Scale up so the grain detail is visible on the small log meshes
    vec3 wp = pos * 8.0;

    // Distorted distance from a virtual trunk axis for ring pattern
    float dx = wp.x + fbm3(wp * 0.4) * 2.5;
    float dz = wp.z + fbm3(wp * 0.4 + 50.0) * 2.5;
    float dist = sqrt(dx * dx + dz * dz);

    // Growth rings
    float rings = sin(dist * 3.5) * 0.5 + 0.5;
    rings = pow(rings, 0.7);

    // Fine grain along the log length (y axis)
    float grain = noise3D(vec3(wp.x * 2.0, wp.y * 25.0, wp.z * 2.0));
    float fineGrain = noise3D(vec3(wp.x * 5.0, wp.y * 40.0, wp.z * 5.0));

    // Three shades of wood
    vec3 lightWood  = vec3(0.65, 0.42, 0.18);
    vec3 mediumWood = vec3(0.45, 0.26, 0.10);
    vec3 darkWood   = vec3(0.28, 0.14, 0.05);

    // Blend the shades using ring and grain patterns
    vec3 baseColor = mix(lightWood, mediumWood, rings * 0.7);
    baseColor = mix(baseColor, darkWood, grain * 0.4);
    baseColor -= vec3(0.06, 0.04, 0.02) * fineGrain;

    // Knots / imperfections from low-frequency noise
    float knot = fbm3(wp * 0.3 + 20.0);
    baseColor = mix(baseColor, darkWood * 0.8, smoothstep(0.55, 0.7, knot) * 0.5);

    return clamp(baseColor, 0.0, 1.0);
}

void main(void)
{
    vec3 normal = normalize(fNormal);

    // Point light above the fire pit (warm orange tone)
    vec3 lightPos = vec3(0.0, 2.5, 0.0);
    vec3 lightColor = vec3(1.0, 0.72, 0.35);

    vec3 toLight = lightPos - fWorldPos;
    float lightDist = length(toLight);
    vec3 lightDir = toLight / lightDist;
    float attenuation = 1.0 / (1.0 + 0.1 * lightDist + 0.03 * lightDist * lightDist);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient = vec3(0.12, 0.09, 0.07);

    // Detect wood vs stone by vertex color:
    // logs are brown (red channel much higher than blue)
    // stones are gray (channels roughly equal)
    float rDominance = fColor.r - fColor.b;
    bool isWood = (rDominance > 0.06) && (fColor.r > 0.15);

    vec3 surfaceColor;
    if (isWood) {
        surfaceColor = woodTexture(fWorldPos);
    } else {
        // Brighten stones/charcoal a bit so they read well against the dark ground
        surfaceColor = fColor * 2.2 + vec3(0.04, 0.03, 0.03);
    }

    // Diffuse + ambient lighting
    vec3 litColor = ambient * surfaceColor
                  + diff * attenuation * lightColor * surfaceColor * 1.5;

    // Warm rim highlight on edges facing away from the fire
    float rim = 1.0 - max(dot(normal, lightDir), 0.0);
    litColor += vec3(0.8, 0.35, 0.1) * pow(rim, 3.0) * 0.1 * attenuation;

    // Gamma correction
    litColor = pow(litColor, vec3(1.0 / 1.6));

    fragColor = vec4(litColor, 1.0);
}
