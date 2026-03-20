/*
Description:
	COSC 3307 - Final Project: Real-Time Particle Systems
	Fire, Smoke, and Fireworks

	Foundation code providing:
	  - OpenGL window and context setup (GLEW + GLFW)
	  - Camera with full movement controls
	  - Shader loading and uniform helper functions
	  - Main render loop with delta time

	Particle system features:
	  - Fire effect: upward-rising glowing particles with turbulence
	  - Smoke effect: slow-rising, expanding gray particles
	  - Fireworks effect: rocket launch with spherical burst explosion
	  - Ground plane with distance-based fade

	Controls:
	  Camera:
	    UP/DOWN arrows    - Pitch (rotate around x-axis)
	    LEFT/RIGHT arrows - Yaw (rotate around y-axis)
	    A/D               - Roll (rotate around z-axis)
	    W/S               - Move forward/backward along look-at vector

	  Particle System:
	    1 - Switch to Fire effect
	    2 - Switch to Smoke effect
	    3 - Switch to Fireworks effect
	    F - Launch a firework (in fireworks mode)
	    +/= - Increase emission rate
	    -   - Decrease emission rate

	  Q - Quit the program

Author: Joachim Brian - 0686270
*/


#include <iostream>
#include <stdexcept>
#include <string>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <camera.h>
#include <particle.h>

#include <fstream>
#include <sstream>
#include <vector>

// Path to the resource/ directory containing shaders and textures
#define SHADER_DIRECTORY "C:/Users/Brian Joachim/Desktop/FinalProject 3D/finalproject/resource/"

// Macro for printing exceptions
#define PrintException(exception_object)\
	std::cerr << exception_object.what() << std::endl

// ================================================================
// Window and viewport settings
// ================================================================
const std::string window_title_g = "COSC 3307 - Final Project: Particle Systems";
const unsigned int window_width_g = 1200;
const unsigned int window_height_g = 800;
const glm::vec3 background(0.02f, 0.02f, 0.05f); // Dark night sky

// ================================================================
// Camera settings
// ================================================================
float camera_near_clip_distance_g = 0.1f;
float camera_far_clip_distance_g = 1000.0f;
float camera_fov_g = 60.0f;

// ================================================================
// Transformation matrices
// ================================================================
glm::mat4 view_matrix, projection_matrix;

// ================================================================
// Global objects
// ================================================================
GLFWwindow* window;
Camera* camera;

// ================================================================
// Particle system (global so KeyCallback can access it)
// ================================================================
ParticleSystem* particleSystem;

// ================================================================
// Vertex structure
// ================================================================
struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
};

// ================================================================
// Geometry structure for OpenGL buffers
// ================================================================
typedef struct Geometry {
	GLuint vbo; // OpenGL vertex buffer object
	GLuint ibo; // OpenGL index buffer object
	GLuint vao; // OpenGL vertex array object
	GLuint size; // Number of elements to draw
} Geometry;


// ================================================================
// LoadTextFile: Reads a file into a string (used for shader source)
// ================================================================
std::string LoadTextFile(std::string filename) {

	const char* char_file = filename.c_str();
	std::ifstream f;
	f.open(char_file);
	if (f.fail()) {
		throw(std::ios_base::failure(std::string("Error opening file ") + std::string(filename)));
	}

	std::string content;
	std::string line;
	while (std::getline(f, line)) {
		content += line + "\n";
	}

	f.close();
	return content;
}


// ================================================================
// LoadShaders: Compiles and links a vertex/fragment shader program
//   Takes a base name and loads <name>.vert and <name>.frag
// ================================================================
GLuint LoadShaders(std::string shaderName) {

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);

	std::string fragment_shader = LoadTextFile(std::string(SHADER_DIRECTORY) + shaderName + ".frag");
	std::string vertex_shader = LoadTextFile(std::string(SHADER_DIRECTORY) + shaderName + ".vert");

	// Compile vertex shader
	const char* const_vertex_shader = vertex_shader.c_str();
	glShaderSource(vs, 1, &const_vertex_shader, NULL);
	glCompileShader(vs);

	GLint status;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(vs, 512, NULL, buffer);
		throw(std::ios_base::failure(std::string("Error compiling vertex shader: ") + std::string(buffer)));
	}

	// Compile fragment shader
	const char* const_fragment_shader = fragment_shader.c_str();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &const_fragment_shader, NULL);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(fs, 512, NULL, buffer);
		throw(std::ios_base::failure(std::string("Error compiling fragment shader: ") + std::string(buffer)));
	}

	// Link shader program
	GLuint shader = glCreateProgram();
	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(shader, 512, NULL, buffer);
		throw(std::ios_base::failure(std::string("Error linking shaders: ") + std::string(buffer)));
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader;
}


// ================================================================
// Uniform helper functions
// ================================================================
void LoadShaderMatrix(GLuint shader, glm::mat4 matrix, std::string name) {
	GLint loc = glGetUniformLocation(shader, name.c_str());
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void LoadShaderVec3(GLuint shader, glm::vec3 vec, std::string name) {
	GLint loc = glGetUniformLocation(shader, name.c_str());
	glUniform3fv(loc, 1, glm::value_ptr(vec));
}

void LoadShaderFloat(GLuint shader, float val, std::string name) {
	GLint loc = glGetUniformLocation(shader, name.c_str());
	glUniform1f(loc, val);
}

void LoadShaderInt(GLuint shader, int val, std::string name) {
	GLint loc = glGetUniformLocation(shader, name.c_str());
	glUniform1i(loc, val);
}


// ================================================================
// PrintOpenGLInfo: Displays renderer information
// ================================================================
void PrintOpenGLInfo(void) {
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}


// ================================================================
// KeyCallback: Handles keyboard input
// ================================================================
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

	// Quit
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	// Camera rotation (compensates for GLM degree/radian double-conversion)
	float rot_factor = 180.0f / glm::pi<float>();
	float trans_factor = 1.0f;

	// Pitch: UP/DOWN arrows
	if (key == GLFW_KEY_UP)    camera->Pitch(rot_factor);
	if (key == GLFW_KEY_DOWN)  camera->Pitch(-rot_factor);

	// Yaw: LEFT/RIGHT arrows
	if (key == GLFW_KEY_LEFT)  camera->Yaw(-rot_factor);
	if (key == GLFW_KEY_RIGHT) camera->Yaw(rot_factor);

	// Roll: A/D keys
	if (key == GLFW_KEY_A) camera->Roll(rot_factor);
	if (key == GLFW_KEY_D) camera->Roll(-rot_factor);

	// Forward/Backward: W/S keys
	if (key == GLFW_KEY_W) camera->MoveForward(trans_factor);
	if (key == GLFW_KEY_S) camera->MoveBackward(trans_factor);

	// ---- Particle system controls ----

	// Press 1 to switch to Fire effect
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		particleSystem->SetEffect(EFFECT_FIRE);
		std::cout << "Effect: Fire (emission rate: " << particleSystem->GetEmissionRate() << ")" << std::endl;
	}

	// Press 2 to switch to Smoke effect
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		particleSystem->SetEffect(EFFECT_SMOKE);
		std::cout << "Effect: Smoke (emission rate: " << particleSystem->GetEmissionRate() << ")" << std::endl;
	}

	// Press 3 to switch to Fireworks effect
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		particleSystem->SetEffect(EFFECT_FIREWORKS);
		std::cout << "Effect: Fireworks" << std::endl;
	}

	// Press F to manually launch a firework (only in fireworks mode)
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		if (particleSystem->GetEffect() == EFFECT_FIREWORKS) {
			particleSystem->TriggerFirework();
			std::cout << "Firework launched!" << std::endl;
		}
	}

	// Press +/= to increase emission rate (for fire/smoke)
	if (key == GLFW_KEY_EQUAL) {
		float rate = particleSystem->GetEmissionRate() + 20.0f;
		particleSystem->SetEmissionRate(rate);
		std::cout << "Emission rate: " << particleSystem->GetEmissionRate() << std::endl;
	}

	// Press - to decrease emission rate (for fire/smoke)
	if (key == GLFW_KEY_MINUS) {
		float rate = particleSystem->GetEmissionRate() - 20.0f;
		particleSystem->SetEmissionRate(rate);
		std::cout << "Emission rate: " << particleSystem->GetEmissionRate() << std::endl;
	}
}


// ================================================================
// ResizeCallback: Handles window resize
// ================================================================
void ResizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	void* ptr = glfwGetWindowUserPointer(window);
	glm::mat4 *projection_matrix = (glm::mat4 *) ptr;
	float top = tan((camera_fov_g / 2.0f) * (glm::pi<float>() / 180.0f)) * camera_near_clip_distance_g;
	float right = top * width / height;
	(*projection_matrix) = glm::frustum(-right, right, -top, top, camera_near_clip_distance_g, camera_far_clip_distance_g);
}


// ================================================================
// CreateGroundPlane: Builds a flat grid to serve as the ground
// ================================================================
// The ground is a grid of triangles centered at the origin.
// It gives the scene a sense of space and orientation.
// Each vertex has a dark color that fades at the edges (handled by shader).
Geometry CreateGroundPlane(float size, int divisions) {

	Geometry ground;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	// Step size: how far apart each grid line is
	float step = (2.0f * size) / (float)divisions;

	// Create grid vertices
	// We loop through rows (z) and columns (x) to make a flat plane at y=0
	for (int z = 0; z <= divisions; z++) {
		for (int x = 0; x <= divisions; x++) {
			Vertex v;
			// Position: centered at origin, stretching from -size to +size
			v.pos = glm::vec3(
				-size + x * step,  // x coordinate
				0.0f,              // y = 0 (flat on the ground)
				-size + z * step   // z coordinate
			);
			// Dark ground color with subtle variation
			float shade = 0.05f + 0.03f * ((x + z) % 2); // checkerboard-like subtle pattern
			v.color = glm::vec3(shade, shade + 0.02f, shade + 0.04f);
			// Normal points straight up
			v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			vertices.push_back(v);
		}
	}

	// Create triangle indices
	// Each grid cell is made of 2 triangles (6 indices per cell)
	for (int z = 0; z < divisions; z++) {
		for (int x = 0; x < divisions; x++) {
			int topLeft     = z * (divisions + 1) + x;
			int topRight    = topLeft + 1;
			int bottomLeft  = (z + 1) * (divisions + 1) + x;
			int bottomRight = bottomLeft + 1;

			// First triangle (top-left, bottom-left, top-right)
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			// Second triangle (top-right, bottom-left, bottom-right)
			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	// Upload to GPU using OpenGL buffer objects
	// VAO = Vertex Array Object (stores the vertex format configuration)
	// VBO = Vertex Buffer Object (stores actual vertex data)
	// IBO = Index Buffer Object (stores which vertices form each triangle)
	glGenVertexArrays(1, &ground.vao);
	glBindVertexArray(ground.vao);

	glGenBuffers(1, &ground.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ground.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &ground.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ground.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	// Tell OpenGL how to read the vertex data:
	// Attribute 0 = position (3 floats, starting at offset 0 in each Vertex)
	GLint posAttrib = 0;
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

	// Attribute 1 = color (3 floats, starting at offset of 'color' in each Vertex)
	GLint colAttrib = 1;
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	// Attribute 2 = normal (3 floats, starting at offset of 'normal' in each Vertex)
	GLint normAttrib = 2;
	glEnableVertexAttribArray(normAttrib);
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	// Store the number of indices so we know how many to draw later
	ground.size = static_cast<GLuint>(indices.size());

	glBindVertexArray(0);
	return ground;
}


// ================================================================
// Main function
// ================================================================
int main(void) {

	try {
		// Initialize GLFW
		if (!glfwInit()) {
			throw(std::runtime_error(std::string("Could not initialize the GLFW library")));
		}

		#ifdef __APPLE__
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		#endif

		// Create window
		window = glfwCreateWindow(window_width_g, window_height_g, window_title_g.c_str(), NULL, NULL);
		if (!window) {
			glfwTerminate();
			throw(std::runtime_error(std::string("Could not create window")));
		}

		glfwMakeContextCurrent(window);

		// Initialize GLEW
		glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		if (err != GLEW_OK) {
			throw(std::runtime_error(std::string("Could not initialize the GLEW library: ") + std::string((const char *)glewGetErrorString(err))));
		}

		// OpenGL state
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDisable(GL_CULL_FACE);

		// ----------------------------------------
		// Camera setup
		// ----------------------------------------
		camera = new Camera();
		camera->SetCamera(
			glm::vec3(0.0f, 5.0f, 20.0f),   // Position: slightly above and back
			glm::vec3(0.0f, 3.0f, 0.0f),     // Look-at: slightly above origin
			glm::vec3(0.0f, 1.0f, 0.0f)      // Up vector
		);
		camera->SetPerspectiveView(camera_fov_g, (float)window_width_g / (float)window_height_g,
			camera_near_clip_distance_g, camera_far_clip_distance_g);

		// ----------------------------------------
		// Load shaders
		// ----------------------------------------
		// Load the particle shader (used for rendering point-sprite particles)
		GLuint particleShader = LoadShaders("particle");

		// Load the ground shader (used for the ground plane with distance fade)
		GLuint groundShader = LoadShaders("ground");

		// ----------------------------------------
		// Create ground plane geometry
		// ----------------------------------------
		// A 100x100 unit grid with 40 subdivisions, centered at the origin
		Geometry groundPlane = CreateGroundPlane(50.0f, 40);

		// ----------------------------------------
		// Create particle system
		// ----------------------------------------
		// Start with Fire effect, emitter at world origin, up to 5000 particles
		particleSystem = new ParticleSystem(EFFECT_FIRE, glm::vec3(0.0f, 0.0f, 0.0f), 5000);

		// ----------------------------------------
		// Set up callbacks
		// ----------------------------------------
		glfwSetWindowUserPointer(window, (void *)&projection_matrix);
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetFramebufferSizeCallback(window, ResizeCallback);

		PrintOpenGLInfo();

		std::cout << "\n=== COSC 3307 Final Project: Particle Systems ===" << std::endl;
		std::cout << "Camera: Arrow keys (pitch/yaw), A/D (roll), W/S (forward/backward)" << std::endl;
		std::cout << "Effects: 1 = Fire, 2 = Smoke, 3 = Fireworks" << std::endl;
		std::cout << "Controls: +/- = emission rate, F = launch firework" << std::endl;
		std::cout << "Q: Quit" << std::endl;
		std::cout << "=================================================\n" << std::endl;

		// ----------------------------------------
		// Delta time tracking
		// ----------------------------------------
		float delta_time;
		float last_time = glfwGetTime();

		// ----------------------------------------
		// Main render loop
		// ----------------------------------------
		while (!glfwWindowShouldClose(window)) {

			// Calculate delta time for frame-rate independent updates
			float current_time = glfwGetTime();
			delta_time = current_time - last_time;
			last_time = current_time;

			// Clear screen
			glClearColor(background[0], background[1], background[2], 0.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Get camera matrices
			view_matrix = camera->GetViewMatrix(NULL);
			projection_matrix = camera->GetProjectionMatrix(NULL);

			// ============================================================
			// STEP 1: Update particle system (physics, lifecycle, spawn)
			// ============================================================
			// This advances all particle positions, spawns new ones,
			// and removes dead ones based on elapsed time (delta_time)
			particleSystem->Update(delta_time);

			// ============================================================
			// STEP 2: Render the ground plane
			// ============================================================
			// The ground gives the scene a sense of space and orientation.
			// We use the ground shader which has a distance-based fade effect.
			{
				glUseProgram(groundShader);

				// Send the camera matrices to the ground shader
				// These transform vertices from world space -> screen space
				LoadShaderMatrix(groundShader, view_matrix, "view_mat");
				LoadShaderMatrix(groundShader, projection_matrix, "projection_mat");

				// The ground sits at the origin with no extra transformation
				glm::mat4 world_mat = glm::mat4(1.0f); // identity matrix = no transformation
				LoadShaderMatrix(groundShader, world_mat, "world_mat");

				// Enable alpha blending so the ground edges can fade out
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				// Draw the ground plane triangles
				glBindVertexArray(groundPlane.vao);
				glDrawElements(GL_TRIANGLES, groundPlane.size, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);

				glDisable(GL_BLEND);
			}

			// ============================================================
			// STEP 3: Render particles (with additive blending for glow)
			// ============================================================
			// Particles are rendered AFTER the ground so they appear on top.
			// The particle system handles all the blending setup internally.
			particleSystem->Render(particleShader, view_matrix, projection_matrix);

			// Swap buffers and poll events
			glfwPollEvents();
			glfwSwapBuffers(window);
		}

		// ----------------------------------------
		// Cleanup: free allocated memory
		// ----------------------------------------
		delete particleSystem;
		delete camera;
	}
	catch (std::exception &e) {
		PrintException(e);
	}

	return 0;
}
