/*
Description:
	COSC 3307 - Final Project: Real-Time Particle Systems
	Fire, Smoke, and Fireworks

	Foundation code providing:
	  - OpenGL window and context setup (GLEW + GLFW)
	  - Camera with full movement controls
	  - Shader loading and uniform helper functions
	  - Main render loop with delta time

	Controls:
	  Camera:
	    UP/DOWN arrows    - Pitch (rotate around x-axis)
	    LEFT/RIGHT arrows - Yaw (rotate around y-axis)
	    A/D               - Roll (rotate around z-axis)
	    W/S               - Move forward/backward along look-at vector

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

	// TODO: Add particle system controls here
	// e.g., 1/2/3 to switch effects, +/- for emission rate, etc.
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
		// TODO: Load particle shaders, ground shader, etc.
		// GLuint particleShader = LoadShaders("particle");
		// GLuint groundShader = LoadShaders("ground");

		// ----------------------------------------
		// Set up callbacks
		// ----------------------------------------
		glfwSetWindowUserPointer(window, (void *)&projection_matrix);
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetFramebufferSizeCallback(window, ResizeCallback);

		PrintOpenGLInfo();

		std::cout << "\n=== COSC 3307 Final Project: Particle Systems ===" << std::endl;
		std::cout << "Camera: Arrow keys (pitch/yaw), A/D (roll), W/S (forward/backward)" << std::endl;
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

			// TODO: Update and render particle systems here
			// 1. Update particles (physics, lifecycle, spawn/destroy)
			// 2. Render ground plane
			// 3. Render particles (with blending)

			// Swap buffers and poll events
			glfwPollEvents();
			glfwSwapBuffers(window);
		}
	}
	catch (std::exception &e) {
		PrintException(e);
	}

	return 0;
}
