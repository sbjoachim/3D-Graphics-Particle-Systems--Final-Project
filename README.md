# COSC 3307 Final Project: Real-Time Particle Systems

**Author:** Joachim Brian (Student ID: 0686270)

## Overview

A real-time 3D particle effects engine built from scratch using **OpenGL**, **GLSL shaders**, and **C++17**. The application demonstrates three distinct visual effects — fire, smoke, and fireworks — each with unique physics, rendering, and emission behaviors.

Particles are rendered as **point sprites** with soft circular falloff in the fragment shader. Fire and fireworks use **additive blending** for a natural glow where particles overlap, while smoke uses standard alpha blending for realistic opacity.

## Features

- **Fire Effect** — Upward-rising flame particles with turbulence, color transitions (yellow to red), and fade-out over a short lifespan.
- **Smoke Effect** — Slow-rising, expanding gray particles with gentle wind drift and longer lifespan.
- **Fireworks Effect** — Multi-phase rockets with trail particles, spherical burst explosions in random colors, gravity-affected arcs, and multiple simultaneous launches.
- **Ground Plane** — Subdivided grid with distance-based fade for spatial orientation.
- **6-DOF Camera** — Quaternion-based rotation (pitch, yaw, roll) with forward/backward movement.
- **Object Pool Architecture** — Pre-allocated 5000-particle pool for zero-allocation real-time performance.
- **Frame-Rate Independent** — All physics use delta-time for consistent behavior at any FPS.

## Controls

| Key              | Action                                  |
|------------------|-----------------------------------------|
| `UP` / `DOWN`    | Pitch camera                            |
| `LEFT` / `RIGHT` | Yaw camera                              |
| `A` / `D`        | Roll camera                             |
| `W` / `S`        | Move camera forward / backward          |
| `1`              | Switch to Fire effect                   |
| `2`              | Switch to Smoke effect                  |
| `3`              | Switch to Fireworks effect              |
| `F`              | Manually launch a firework              |
| `+` / `-`        | Increase / decrease emission rate       |
| `Q`              | Quit                                    |

## Architecture

```
main.cpp          - Window setup, render loop, input handling, shader loading
include/camera.h  - Camera class with quaternion orientation
include/particle.h - Particle struct, ParticleSystem class, EffectType enum
src/camera.cpp    - Camera movement and view/projection matrix construction
src/particle.cpp  - Emission, physics, lifecycle, and GPU rendering for all effects
resource/*.vert   - GLSL vertex shaders (particle positioning, ground MVP)
resource/*.frag   - GLSL fragment shaders (soft circles, distance fade)
```

## Build Instructions

### Prerequisites
- CMake 3.16+
- C++17 compiler (MSVC, GCC, or Clang)
- OpenGL 3.3+
- GLEW, GLFW3, SOIL libraries

### Build (Windows with Visual Studio)
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Build (Linux/Mac)
```bash
mkdir build && cd build
cmake ..
make
```

### Run
```bash
./build/Release/ParticleSystems.exe    # Windows
./build/ParticleSystems                # Linux/Mac
```

## Technical Details

- **Rendering**: GL_POINTS with `gl_PointSize` for screen-space scaling by distance
- **Blending**: Additive (`GL_SRC_ALPHA, GL_ONE`) for fire/fireworks glow; standard alpha for smoke
- **Shaders**: GLSL 3.30 with `smoothstep` soft-circle falloff and distance-based ground fade
- **Physics**: Euler integration with effect-specific forces (turbulence, gravity, wind)
- **Memory**: Object pool pattern — no runtime allocation in the render loop
