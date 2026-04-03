/*
Description:
	Particle System implementation for COSC 3307 Final Project
	Handles three visual effects: Fire, Smoke, and Fireworks

	HOW IT WORKS (high-level overview):
	===================================
	A particle system manages a pool of "particles" — tiny objects that
	each have a position, velocity, color, size, and lifespan.

	Every frame, we:
	  1. EMIT new particles (create them at the emitter position)
	  2. UPDATE existing particles (move them, apply physics, age them)
	  3. REMOVE dead particles (ones whose life has reached zero)
	  4. RENDER surviving particles as glowing point-sprites on the GPU

	Different effects (fire, smoke, fireworks) simply change HOW particles
	are emitted and HOW they behave during the update step.

Author: Joachim Brian - 0686270
*/

#include <particle.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>


// ================================================================
// Constructor
// ================================================================
// Creates a particle system with a given effect type, emitter position,
// and maximum number of particles that can exist at once.
//
// We pre-allocate ALL particles up front (object pooling pattern).
// Instead of creating/destroying particles dynamically, we just mark
// them as "active" or "inactive". This avoids expensive memory
// allocation during the real-time render loop.
ParticleSystem::ParticleSystem(EffectType type, glm::vec3 emitterPos, int maxParticles)
	: effectType_(type), emitterPos_(emitterPos), maxParticles_(maxParticles),
	  emissionRate_(100.0f), emissionAccumulator_(0.0f),
	  autoLaunchTimer_(0.0f), activeBurstCount_(0), vao_(0), vbo_(0)
{
	// Create the particle pool — all start as inactive
	particles_.resize(maxParticles_);
	for (auto& p : particles_) {
		p.active = false;
	}

	// Set up the OpenGL buffers we'll use for rendering
	InitBuffers();
}


// ================================================================
// Destructor
// ================================================================
// Clean up GPU resources when the particle system is destroyed.
// OpenGL buffers persist on the GPU until explicitly deleted.
ParticleSystem::~ParticleSystem() {
	if (vbo_) glDeleteBuffers(1, &vbo_);
	if (vao_) glDeleteVertexArrays(1, &vao_);
}


// ================================================================
// InitBuffers: Set up OpenGL vertex array/buffer objects
// ================================================================
// We use "point sprites" to render particles. Each particle is a
// single point (GL_POINTS) that the GPU expands into a small square.
// The fragment shader then shapes it into a soft circle.
//
// The VBO (Vertex Buffer Object) stores per-particle data:
//   - position (vec3): where in 3D space
//   - color (vec4):    RGBA color with transparency
//   - size (float):    how large the point appears
//
// The VAO (Vertex Array Object) remembers how to read this data
// so we don't have to reconfigure it every frame.
//
// GL_DYNAMIC_DRAW tells OpenGL we'll be updating this buffer frequently
// (every frame), so it should optimize for writes.
void ParticleSystem::InitBuffers() {
	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);

	// Allocate GPU memory for the maximum number of particles
	glBufferData(GL_ARRAY_BUFFER, maxParticles_ * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);

	// Tell OpenGL how to interpret the data in the buffer:

	// Attribute 0: position (3 floats: x, y, z)
	// This maps to "layout(location = 0) in vec3 position" in the vertex shader
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
		(void*)offsetof(ParticleVertex, position));

	// Attribute 1: color (4 floats: r, g, b, a)
	// This maps to "layout(location = 1) in vec4 color" in the vertex shader
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
		(void*)offsetof(ParticleVertex, color));

	// Attribute 2: size (1 float)
	// This maps to "layout(location = 2) in float psize" in the vertex shader
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
		(void*)offsetof(ParticleVertex, size));

	// Unbind the VAO so other code doesn't accidentally modify our setup
	glBindVertexArray(0);
}


// ================================================================
// RandomFloat: Returns a random decimal number between min and max
// ================================================================
// Used heavily to add natural variation to particle properties.
// Without randomness, all particles would look identical and move
// in the exact same way, which looks very artificial.
float ParticleSystem::RandomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}


// ================================================================
// EmitFireParticle: Initialize one particle with fire properties
// ================================================================
// Fire particles:
//   - Spawn near the ground (at the emitter position)
//   - Move upward with some random horizontal drift
//   - Are colored bright orange/yellow (like real flames)
//   - Have a short lifespan (0.5-1.5 seconds)
//   - Start large and shrink as they rise and fade
void ParticleSystem::EmitFireParticle(Particle& p) {
	// Spread particles slightly around the emitter so the fire has width
	float spread = 0.8f;
	p.position = emitterPos_ + glm::vec3(
		RandomFloat(-spread, spread),  // random x offset
		RandomFloat(-0.1f, 0.2f),      // near ground level
		RandomFloat(-spread, spread)   // random z offset
	);

	// Velocity: mainly upward, with slight horizontal wobble
	// This creates the flickering, rising motion of flames
	p.velocity = glm::vec3(
		RandomFloat(-0.3f, 0.3f),      // slight left/right drift
		RandomFloat(2.0f, 5.0f),       // strong upward motion
		RandomFloat(-0.3f, 0.3f)       // slight front/back drift
	);

	// Color: layered flame palette — occasional white-hot core particles
	// near the emitter base, with orange/yellow mid-flame and deep red tips
	float coreChance = RandomFloat(0.0f, 1.0f);
	if (coreChance < 0.15f) {
		// White-hot ember near the base (brightest part of the flame)
		p.color = glm::vec3(1.0f, 0.92f, 0.7f);
		p.size = RandomFloat(8.0f, 16.0f);
		p.life = RandomFloat(0.3f, 0.7f);
	} else if (coreChance < 0.5f) {
		// Bright yellow-orange mid-flame
		p.color = glm::vec3(1.0f, RandomFloat(0.55f, 0.75f), RandomFloat(0.0f, 0.08f));
		p.size = RandomFloat(14.0f, 26.0f);
		p.life = RandomFloat(0.5f, 1.2f);
	} else {
		// Deep orange to red outer flame
		p.color = glm::vec3(RandomFloat(0.85f, 1.0f), RandomFloat(0.2f, 0.45f), RandomFloat(0.0f, 0.05f));
		p.size = RandomFloat(16.0f, 32.0f);
		p.life = RandomFloat(0.8f, 1.6f);
	}

	p.alpha = 1.0f;                        // fully opaque when born
	p.maxLife = p.life;                    // remember original lifespan for fade calculation
	p.active = true;                       // mark as alive
}


// ================================================================
// EmitSmokeParticle: Initialize one particle with smoke properties
// ================================================================
// Smoke particles:
//   - Spawn above the fire (higher starting y position)
//   - Rise slowly with gentle horizontal drift
//   - Are gray colored (various shades)
//   - Live longer than fire particles (2-4 seconds)
//   - Start semi-transparent and grow larger as they rise
void ParticleSystem::EmitSmokeParticle(Particle& p) {
	float spread = 0.5f;
	p.position = emitterPos_ + glm::vec3(
		RandomFloat(-spread, spread),
		RandomFloat(2.0f, 3.0f),       // starts above the fire
		RandomFloat(-spread, spread)
	);

	// Slower upward velocity than fire, with gentle wander
	p.velocity = glm::vec3(
		RandomFloat(-0.5f, 0.5f),      // horizontal drift
		RandomFloat(1.0f, 2.5f),       // gentle upward rise
		RandomFloat(-0.5f, 0.5f)       // horizontal drift
	);

	// Warm-tinted smoke near the fire base, cooling to neutral gray as it rises
	// Slight color variation prevents flat, uniform look
	float gray = RandomFloat(0.25f, 0.55f);
	float warmth = RandomFloat(0.0f, 1.0f);
	if (warmth < 0.35f) {
		// Warm brown-gray (near fire, recently heated)
		p.color = glm::vec3(gray + 0.08f, gray + 0.03f, gray - 0.02f);
	} else {
		// Cool blue-gray (higher, cooled smoke)
		p.color = glm::vec3(gray - 0.02f, gray, gray + 0.04f);
	}

	p.alpha = RandomFloat(0.4f, 0.65f);   // varied transparency for depth
	p.life = RandomFloat(2.0f, 4.0f);     // smoke lingers longer than fire
	p.maxLife = p.life;
	p.size = RandomFloat(22.0f, 50.0f);   // denser, larger smoke puffs
	p.active = true;
}


// ================================================================
// EmitFireworkBurst: Create an explosion of particles at a point
// ================================================================
// When a firework rocket reaches its peak height, this function
// creates ~150 particles that fly outward in all directions,
// simulating a spherical explosion.
//
// MATH: We use spherical coordinates (theta, phi) to distribute
// particles evenly in all directions:
//   theta = angle around the vertical axis (0 to 2*PI)
//   phi   = angle from the top (0 to PI)
//   x = speed * sin(phi) * cos(theta)
//   y = speed * cos(phi)
//   z = speed * sin(phi) * sin(theta)
// This converts a sphere's surface into x,y,z velocity directions.
void ParticleSystem::EmitFireworkBurst(glm::vec3 origin) {
	// Color palette: cycles through distinct colors in fixed order.
	// No rand() involved — pure counter guarantees every color appears.
	static const glm::vec3 colorPalette[] = {
		glm::vec3(1.0f, 0.95f, 0.15f),  // 0: Vivid Yellow
		glm::vec3(0.15f, 0.35f, 1.0f),  // 1: Electric Blue
		glm::vec3(1.0f, 0.45f, 0.05f),  // 2: Amber Orange
		glm::vec3(0.75f, 0.05f, 1.0f),  // 3: Violet Purple
		glm::vec3(1.0f, 0.85f, 0.25f),  // 4: Warm Gold
		glm::vec3(0.1f, 0.8f, 0.95f),   // 5: Cyan
		glm::vec3(1.0f, 0.15f, 0.3f),   // 6: Crimson Rose
		glm::vec3(0.95f, 0.15f, 0.75f), // 7: Hot Pink
		glm::vec3(0.3f, 1.0f, 0.4f),    // 8: Emerald Green
		glm::vec3(1.0f, 0.7f, 0.1f),    // 9: Tangerine
	};
	static const int NUM_COLORS = 10;
	static int colorCounter = 0;

	glm::vec3 burstColor = colorPalette[colorCounter % NUM_COLORS];
	colorCounter++;


	// Find inactive particles and turn them into burst particles
	// 350 particles per burst creates a dense, realistic explosion
	int burstCount = 0;
	for (auto& p : particles_) {
		if (!p.active && burstCount < 350) {
			// Spherical coordinates for uniform distribution
			float theta = RandomFloat(0.0f, 2.0f * 3.14159f);  // horizontal angle
			float phi = RandomFloat(0.0f, 3.14159f);            // vertical angle
			float speed = RandomFloat(2.0f, 6.0f);              // tighter cluster for dense look

			p.position = origin;  // all start at the explosion center

			// Convert spherical to cartesian velocity (outward in all directions)
			p.velocity = glm::vec3(
				speed * sin(phi) * cos(theta),  // x component
				speed * cos(phi),                // y component (some go up, some down)
				speed * sin(phi) * sin(theta)   // z component
			);

			// Add slight color variation so it's not perfectly uniform
			p.color = burstColor + glm::vec3(
				RandomFloat(-0.1f, 0.1f),
				RandomFloat(-0.1f, 0.1f),
				RandomFloat(-0.1f, 0.1f)
			);
			// Clamp to valid color range [0, 1]
			p.color = glm::clamp(p.color, 0.0f, 1.0f);

			p.alpha = 1.0f;

			// 12% of particles become glitter — tiny, bright, longer-lived sparkles
			if (RandomFloat(0.0f, 1.0f) < 0.12f) {
				p.color = glm::vec3(1.0f, 0.98f, 0.9f); // near-white glitter
				p.life = RandomFloat(3.0f, 4.5f);
				p.size = RandomFloat(3.0f, 7.0f);
			} else {
				p.life = RandomFloat(2.0f, 3.5f);
				p.size = RandomFloat(10.0f, 22.0f);
			}

			p.maxLife = p.life;
			p.active = true;
			burstCount++;
		}
	}
}


// ================================================================
// EmitParticle: Dispatch to the correct emitter based on effect type
// ================================================================
void ParticleSystem::EmitParticle(Particle& p) {
	switch (effectType_) {
		case EFFECT_FIRE:
			EmitFireParticle(p);
			break;
		case EFFECT_SMOKE:
			EmitSmokeParticle(p);
			break;
		case EFFECT_FIREWORKS:
			// Fireworks don't use continuous emission.
			// They emit via TriggerFirework() -> EmitFireworkBurst()
			break;
	}
}


// ================================================================
// TriggerFirework: Launch a new firework rocket
// ================================================================
// Launches a new rocket into the sky. Multiple rockets can be active
// simultaneously, creating overlapping bursts for a richer display.
//
// A firework has two phases:
//   Phase 1 (launch): The rocket flies upward, leaving a trail.
//   Phase 2 (burst):  When the rocket slows near its peak,
//                      EmitFireworkBurst() creates the explosion.
void ParticleSystem::TriggerFirework() {
	Rocket r;
	r.position = emitterPos_;  // start from the ground
	r.timer = 0.0f;

	// Reduced upward velocity so the rocket peaks ON SCREEN
	// Camera is at y=5 looking at y=3, so bursts around y=8-15 are visible
	r.velocity = glm::vec3(
		RandomFloat(-2.0f, 2.0f),       // horizontal spread for variety
		RandomFloat(8.0f, 13.0f),       // moderate upward — peaks in view
		RandomFloat(-2.0f, 2.0f)        // horizontal spread for variety
	);

	activeRockets_.push_back(r);
}


// ================================================================
// Update: Main simulation step — called every frame
// ================================================================
// This is the heart of the particle system. It handles:
//   1. Firework rocket movement (if in firework mode)
//   2. Spawning new particles (emission)
//   3. Updating physics for all active particles
//   4. Killing particles whose life has expired
//
// deltaTime is the time (in seconds) since the last frame.
// We multiply all velocities by deltaTime so the simulation
// runs at the same speed regardless of frame rate.
void ParticleSystem::Update(float deltaTime) {
	// Earth's gravity: 9.81 m/s^2 downward
	// Used for firework particles to create realistic arcs
	glm::vec3 gravity(0.0f, -9.81f, 0.0f);

	// ==========================================================
	// FIREWORK LAUNCH PHASE (multiple simultaneous rockets)
	// ==========================================================
	// Each active rocket flies upward, leaves a trail, and explodes
	// at its peak. Using a vector allows overlapping bursts.
	if (effectType_ == EFFECT_FIREWORKS) {
		for (int i = static_cast<int>(activeRockets_.size()) - 1; i >= 0; i--) {
			Rocket& r = activeRockets_[i];
			r.timer += deltaTime;

			// Move the rocket: position += velocity * time
			r.position += r.velocity * deltaTime;

			// Apply reduced gravity (rockets fight gravity on the way up)
			r.velocity += gravity * deltaTime * 0.3f;

			// Emit a trail particle behind the rocket each frame
			for (auto& p : particles_) {
				if (!p.active) {
					p.position = r.position;
					p.velocity = glm::vec3(
						RandomFloat(-0.5f, 0.5f),    // slight spread
						RandomFloat(-1.0f, 0.5f),    // mostly downward (trail falls behind)
						RandomFloat(-0.5f, 0.5f)
					);
					p.color = glm::vec3(1.0f, RandomFloat(0.8f, 0.95f), RandomFloat(0.4f, 0.65f));
					p.alpha = RandomFloat(0.7f, 1.0f);
					p.life = RandomFloat(0.2f, 0.45f);
					p.maxLife = p.life;
					p.size = RandomFloat(3.0f, 6.0f);
					p.active = true;
					break;  // only one trail particle per rocket per frame
				}
			}

			// Check if the rocket reached its peak or timed out
			// Burst at velocity 3.0 or after 1.0s so explosion happens on screen
			if (r.velocity.y <= 3.0f || r.timer > 1.0f) {
				// BOOM! Create the spherical burst at the rocket's position
				EmitFireworkBurst(r.position);
				// Remove this rocket from the active list
				activeRockets_.erase(activeRockets_.begin() + i);
			}
		}
	}

	// ==========================================================
	// CONTINUOUS EMISSION (Fire and Smoke only)
	// ==========================================================
	// For fire and smoke, we continuously spawn new particles.
	// emissionRate_ controls how many particles per second.
	//
	// We use an "accumulator" pattern:
	//   - Each frame, add (rate * deltaTime) to the accumulator
	//   - The integer part tells us how many particles to spawn
	//   - The fractional part carries over to the next frame
	// This ensures smooth emission even at varying frame rates.
	if (effectType_ != EFFECT_FIREWORKS) {
		emissionAccumulator_ += emissionRate_ * deltaTime;
		int toEmit = static_cast<int>(emissionAccumulator_);
		emissionAccumulator_ -= toEmit;  // keep the fractional part

		// Find inactive particles and emit new ones
		for (auto& p : particles_) {
			if (!p.active && toEmit > 0) {
				EmitParticle(p);
				toEmit--;
			}
		}
	}

	// ==========================================================
	// AUTO-LAUNCH FIREWORKS (Assignment 4 inspired spawning)
	// ==========================================================
	// Two-part logic ensures the sky always looks full:
	//   1. Timed launches at random intervals (0.4–1.2s) for natural rhythm
	//   2. Guarantee at least MIN_ACTIVE_BURSTS worth of particles visible
	//      at all times — if the count drops, immediately launch more
	if (effectType_ == EFFECT_FIREWORKS) {
		// Count how many firework particles are currently alive
		int activeFireworkParticles = 0;
		for (const auto& p : particles_) {
			if (p.active) activeFireworkParticles++;
		}

		// If too few particles on screen (bursts have faded), launch immediately
		// Each burst creates ~150 particles, so MIN_ACTIVE_BURSTS * 150 = threshold
		int minParticleThreshold = MIN_ACTIVE_BURSTS * 200;
		if (activeFireworkParticles < minParticleThreshold && activeRockets_.size() < 3) {
			TriggerFirework();
		}

		// Also launch on a regular random timer for natural staggered rhythm
		autoLaunchTimer_ += deltaTime;
		float nextLaunch = 0.4f + static_cast<float>(rand() % 80) / 100.0f; // 0.4–1.2s
		if (autoLaunchTimer_ > nextLaunch) {
			TriggerFirework();
			autoLaunchTimer_ = 0.0f;
		}
	}

	// ==========================================================
	// UPDATE ALL ACTIVE PARTICLES
	// ==========================================================
	// For each active particle:
	//   1. Decrease its remaining life
	//   2. Kill it if life <= 0
	//   3. Apply effect-specific physics (turbulence, gravity, etc.)
	//   4. Move it (position += velocity * deltaTime)
	for (auto& p : particles_) {
		if (!p.active) continue;  // skip dead particles

		// Age the particle
		p.life -= deltaTime;
		if (p.life <= 0.0f) {
			p.active = false;  // kill it
			continue;
		}

		// lifeRatio goes from 1.0 (just born) to 0.0 (about to die)
		// We use this to fade out particles smoothly over their lifetime
		float lifeRatio = p.life / p.maxLife;

		// --- FIRE PHYSICS ---
		if (effectType_ == EFFECT_FIRE) {
			// Push upward slightly (hot air rises faster over time)
			p.velocity.y += deltaTime * 1.5f;

			// Add random horizontal turbulence (the flickering of flames)
			p.velocity.x += RandomFloat(-2.0f, 2.0f) * deltaTime;
			p.velocity.z += RandomFloat(-2.0f, 2.0f) * deltaTime;

			// Fade transparency based on remaining life
			p.alpha = lifeRatio;

			// Shift color from yellow toward red as the particle ages
			// (multiplying green by 0.98 each frame gradually removes yellow)
			p.color.g *= (1.0f - 2.0f * deltaTime);
		}
		// --- SMOKE PHYSICS ---
		else if (effectType_ == EFFECT_SMOKE) {
			// Gentle horizontal wander (smoke drifts in the wind)
			p.velocity.x += RandomFloat(-0.3f, 0.3f) * deltaTime;
			p.velocity.z += RandomFloat(-0.3f, 0.3f) * deltaTime;

			// Fade out gradually
			p.alpha = 0.6f * lifeRatio;

			// Smoke puffs grow larger as they rise (they expand and disperse)
			p.size += deltaTime * 5.0f;
		}
		// --- FIREWORK PHYSICS ---
		// Inspired by Assignment 4's continuous expansion (norm * t * speed).
		// Particles keep spreading outward while gravity pulls them into arcs.
		else if (effectType_ == EFFECT_FIREWORKS) {
			// Gentle gravity so particles arc down realistically
			p.velocity += gravity * deltaTime * 0.4f;

			// Gentle outward spread: particles expand slowly so the burst
			// stays dense and visible rather than scattering immediately
			float horizSpeed = sqrt(p.velocity.x * p.velocity.x + p.velocity.z * p.velocity.z);
			if (horizSpeed > 0.1f) {
				float invSpeed = 1.0f / horizSpeed;
				float spreadForce = 0.8f * deltaTime;
				p.velocity.x += (p.velocity.x * invSpeed) * spreadForce;
				p.velocity.z += (p.velocity.z * invSpeed) * spreadForce;
			}

			// Linear fade — particles stay bright longer before fading out
			p.alpha = lifeRatio;

			// Grow slightly over time (Assignment 4: size_factor = 1.0 + t * 0.3)
			p.size += deltaTime * 2.0f;
		}

		// Move the particle: new_position = old_position + velocity * time
		// This is basic Euler integration (the simplest physics update method)
		p.position += p.velocity * deltaTime;
	}
}


// ================================================================
// Render: Draw all active particles to the screen
// ================================================================
// This function:
//   1. Collects all active particles into a compact array
//   2. Uploads that array to the GPU (updates the VBO)
//   3. Configures blending for transparency and glow
//   4. Draws all particles as GL_POINTS (point sprites)
//
// Point sprites are the simplest way to render particles in OpenGL.
// Each particle is a single vertex that the GPU expands into a
// screen-aligned square. The fragment shader then makes it circular.
//
// Additive blending (GL_SRC_ALPHA, GL_ONE) means overlapping
// particles ADD their colors together, creating a bright glow
// effect where many particles overlap (like the core of a fire).
void ParticleSystem::Render(GLuint shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
	// Step 1: Collect active particles into a contiguous array
	// We only upload active particles to avoid wasting GPU bandwidth
	std::vector<ParticleVertex> vertices;
	vertices.reserve(maxParticles_);

	for (const auto& p : particles_) {
		if (!p.active) continue;
		ParticleVertex v;
		v.position = p.position;
		v.color = glm::vec4(p.color, p.alpha);  // combine RGB + alpha into vec4
		v.size = p.size;
		vertices.push_back(v);
	}

	// Nothing to draw if no particles are alive
	if (vertices.empty()) return;



	// Step 2: Upload particle data to the GPU
	// glBufferSubData updates PART of an existing buffer (faster than recreating it)
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		vertices.size() * sizeof(ParticleVertex), vertices.data());

	// Step 3: Activate the particle shader and send camera matrices
	glUseProgram(shader);

	GLint loc;
	loc = glGetUniformLocation(shader, "view_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	loc = glGetUniformLocation(shader, "projection_mat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	// Send effect type to the fragment shader for per-effect visual treatment
	loc = glGetUniformLocation(shader, "effectType");
	glUniform1i(loc, static_cast<int>(effectType_));

	// Step 4: Enable blending — choose mode based on effect type
	// Additive blending (GL_ONE) makes overlapping particles brighter, creating
	// a natural glow effect perfect for fire and fireworks.
	// Standard alpha blending is better for smoke, which should obscure what's behind it.
	glEnable(GL_BLEND);
	if (effectType_ == EFFECT_SMOKE) {
		// Standard alpha blend: smoke obscures the background
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		// Additive blend for fire and fireworks: overlapping particles
		// glow brighter, creating a dense luminous core at the burst center
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}

	// Don't write particles to the depth buffer
	// This prevents particles from blocking each other — they should all be visible
	glDepthMask(GL_FALSE);

	// Tell OpenGL that our vertex shader will set gl_PointSize
	glEnable(GL_PROGRAM_POINT_SIZE);

	// Enable point sprites so gl_PointCoord works in the fragment shader
	// (required in OpenGL compatibility profile on some Intel GPUs)
	glEnable(0x8861); // GL_POINT_SPRITE
	glTexEnvi(0x8861, 0x8862, GL_TRUE); // GL_POINT_SPRITE, GL_COORD_REPLACE

	// Step 5: Draw all particles as points
	glBindVertexArray(vao_);
	glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size()));
	glBindVertexArray(0);

	// Step 6: Restore OpenGL state to defaults
	// This is important so other rendering (like the ground plane) isn't affected
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_PROGRAM_POINT_SIZE);
	glDisable(0x8861); // GL_POINT_SPRITE
}


// ================================================================
// SetEffect: Switch to a different particle effect
// ================================================================
// Resets all particles and state so the new effect starts clean
void ParticleSystem::SetEffect(EffectType type) {
	effectType_ = type;

	// Deactivate all existing particles
	for (auto& p : particles_) {
		p.active = false;
	}

	// Reset firework and emission state
	activeRockets_.clear();
	autoLaunchTimer_ = 0.0f;
	activeBurstCount_ = 0;
	emissionAccumulator_ = 0.0f;

	// When switching to fireworks, immediately launch 3 staggered rockets
	// so the sky fills up right away (inspired by Assignment 4's SetupScene)
	if (type == EFFECT_FIREWORKS) {
		for (int i = 0; i < MIN_ACTIVE_BURSTS; i++) {
			TriggerFirework();
		}
	}
}

EffectType ParticleSystem::GetEffect() const {
	return effectType_;
}

// Clamps emission rate between 10 and 500 particles/second
void ParticleSystem::SetEmissionRate(float rate) {
	emissionRate_ = std::max(10.0f, std::min(rate, 500.0f));
}

float ParticleSystem::GetEmissionRate() const {
	return emissionRate_;
}

std::string ParticleSystem::GetEffectName() const {
	switch (effectType_) {
		case EFFECT_FIRE:      return "Fire";
		case EFFECT_SMOKE:     return "Smoke";
		case EFFECT_FIREWORKS: return "Fireworks";
		default:               return "Unknown";
	}
}
