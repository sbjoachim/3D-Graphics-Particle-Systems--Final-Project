/*
Description:
	Particle System header for COSC 3307 Final Project
	Supports Fire, Smoke, and Fireworks effects

	ARCHITECTURE OVERVIEW:
	======================
	- Particle:        A single particle with position, velocity, color, life, etc.
	- ParticleVertex:  A simplified struct sent to the GPU for rendering.
	- ParticleSystem:  Manages a pool of particles, handles emission, physics, and rendering.
	- EffectType:      Enum to switch between Fire, Smoke, and Fireworks modes.

	The system uses an "object pool" pattern — all particles are pre-allocated,
	and we toggle them active/inactive instead of creating/destroying them.
	This is critical for real-time performance.

Author: Joachim Brian - 0686270
*/

#ifndef PARTICLE_H_
#define PARTICLE_H_

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

// ================================================================
// Particle: Represents a single particle in the simulation
// ================================================================
// Each particle has all the data needed for physics simulation
// and visual appearance. Not all of this goes to the GPU —
// only position, color, and size are sent for rendering.
struct Particle {
	glm::vec3 position;   // Current world-space position (x, y, z)
	glm::vec3 velocity;   // Movement direction and speed (units/second)
	glm::vec3 color;      // RGB color (each channel 0.0 to 1.0)
	float alpha;          // Transparency (0.0 = invisible, 1.0 = fully opaque)
	float life;           // Remaining lifespan in seconds
	float maxLife;        // Original lifespan (used to calculate fade ratio)
	float size;           // Visual size of the point sprite
	bool active;          // Is this particle alive? (false = available for reuse)
};

// ================================================================
// EffectType: Which visual effect is currently active
// ================================================================
enum EffectType {
	EFFECT_FIRE = 0,       // Upward-rising orange/yellow flame particles
	EFFECT_SMOKE = 1,      // Slow-rising, expanding gray smoke puffs
	EFFECT_FIREWORKS = 2   // Rocket launch + spherical burst explosion
};

// ================================================================
// ParticleVertex: Data sent to the GPU per particle for rendering
// ================================================================
// This is a compact version of Particle — only the fields the
// shader needs. We pack these into a VBO (Vertex Buffer Object)
// and upload them every frame.
struct ParticleVertex {
	glm::vec3 position;   // World position -> vertex shader attribute 0
	glm::vec4 color;      // RGBA color -> vertex shader attribute 1
	float size;           // Point sprite size -> vertex shader attribute 2
};

// ================================================================
// ParticleSystem: Manages all particles for one effect
// ================================================================
class ParticleSystem {
public:
	// Constructor: specify effect type, emitter position, and max particle count
	ParticleSystem(EffectType type, glm::vec3 emitterPos, int maxParticles);
	~ParticleSystem();

	// Called every frame to advance the simulation by deltaTime seconds
	void Update(float deltaTime);

	// Called every frame to draw all active particles using the given shader
	void Render(GLuint shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

	// Switch to a different effect (resets all particles)
	void SetEffect(EffectType type);
	EffectType GetEffect() const;

	// Control emission speed (particles per second, for fire/smoke)
	void SetEmissionRate(float rate);
	float GetEmissionRate() const;

	// Manually launch a firework (only works in EFFECT_FIREWORKS mode)
	void TriggerFirework();

	// Get the current effect name as a string (for console output)
	std::string GetEffectName() const;

private:
	// ---- Particle pool ----
	std::vector<Particle> particles_;  // Pre-allocated array of all particles
	int maxParticles_;                 // Maximum number of simultaneous particles

	// ---- Emitter settings ----
	EffectType effectType_;            // Current effect mode
	glm::vec3 emitterPos_;            // Where new particles spawn
	float emissionRate_;              // Particles emitted per second
	float emissionAccumulator_;       // Fractional particle counter (for smooth emission)

	// ---- Firework rocket state ----
	bool fireworkLaunched_;           // Is a rocket currently flying up?
	glm::vec3 fireworkPos_;           // Current rocket position
	glm::vec3 fireworkVel_;           // Current rocket velocity
	float fireworkTimer_;             // Time since launch (safety timeout)
	bool fireworkExploded_;           // Has the rocket exploded yet?

	// ---- OpenGL rendering resources ----
	GLuint vao_;                      // Vertex Array Object (stores buffer layout)
	GLuint vbo_;                      // Vertex Buffer Object (stores particle data on GPU)

	// ---- Internal helper methods ----
	void InitBuffers();               // Set up VAO/VBO on the GPU
	void EmitParticle(Particle& p);   // Dispatch to the right emitter
	void EmitFireParticle(Particle& p);     // Configure a particle as fire
	void EmitSmokeParticle(Particle& p);    // Configure a particle as smoke
	void EmitFireworkBurst(glm::vec3 origin); // Create explosion at a point

	float RandomFloat(float min, float max); // Random number utility
};

#endif
