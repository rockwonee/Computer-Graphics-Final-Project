/*#include "skybox.h"
#include "building.h"


#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#include <cstdlib> // for rand() and seeding random numbers for building sizes
#include <ctime>


static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// OpenGL camera view parameters
static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// View control 
static float viewAzimuth = 0.f;
static float viewPolar = 0.f;
static float viewDistance = 500.0f;

int main(void)
{
	Skybox skybox;
	glm::vec3 position(0, 0, 0); // Arrange in a grid
    glm::vec3 scale(500.0f, 500.0f, 500.0f); // Uniform size for skybox
    skybox.initializeSky(position, scale);

	Building building;
	glm::vec3 buildingPoisiton(0, 0, -50);
	glm::vec3 buildingScale(20, 50, 20);
	building.initializeBuilding(buildingPoisiton, buildingScale);
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Lab 2", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	// Background
	glClearColor(0.2f, 0.2f, 0.25f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// I ADDED
	//glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_FRONT); // Cull front faces to render inner surfaces
	// ------

	// TODO: Create more buildings
    // ---------------------------

// Seed the random number generator
srand(static_cast<unsigned int>(time(0)));

int gridSize = 5;
float spacing = 60.0f; // Distance between buildings

		// Random scale values
        //float width = static_cast<float>(rand() % 20 + 10);  // Random width between 10 and 30
        //float height = static_cast<float>(rand() % 50 + 40); // Random height between 40 and 140
        //float depth = static_cast<float>(rand() % 20 + 10);  // Random depth between 10 and 30

	// Camera setup
    //eye_center.y = viewDistance * cos(viewPolar);
    //eye_center.x = viewDistance * cos(viewAzimuth);
    //eye_center.z = viewDistance * sin(viewAzimuth);
	

	eye_center = glm::vec3(0, 0, 20);

	glm::mat4 viewMatrix, projectionMatrix;
    glm::float32 FoV = 75;
	glm::float32 zNear = 0.1f; 
	glm::float32 zFar = 1000.0f;
	projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;

		// Render the building
		skybox.render(vp);

		building.render(vp);
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	
	// Clean up
	skybox.cleanup();
	building.cleanup();
	
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		viewAzimuth = 0.f;
		viewPolar = 0.f;
		eye_center.y = viewDistance * cos(viewPolar);
		eye_center.x = viewDistance * cos(viewAzimuth);
		eye_center.z = viewDistance * sin(viewAzimuth);
		std::cout << "Reset." << std::endl;
	}

	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewPolar -= 0.1f;
		eye_center.y = viewDistance * cos(viewPolar);
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewPolar += 0.1f;
		eye_center.y = viewDistance * cos(viewPolar);
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewAzimuth -= 0.1f;
		eye_center.x = viewDistance * cos(viewAzimuth);
		eye_center.z = viewDistance * sin(viewAzimuth);
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewAzimuth += 0.1f;
		eye_center.x = viewDistance * cos(viewAzimuth);
		eye_center.z = viewDistance * sin(viewAzimuth);
	}

	if (key == GLFW_KEY_Z && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        // Zoom in by reducing viewDistance
        viewDistance -= 5.0f;
        if (viewDistance < 10.0f) viewDistance = 10.0f; // Prevent going too close
        eye_center.y = viewDistance * cos(viewPolar);
        eye_center.x = viewDistance * cos(viewAzimuth);
        eye_center.z = viewDistance * sin(viewAzimuth);
        std::cout << "Zooming in: " << viewDistance << std::endl;
    }

    if (key == GLFW_KEY_X && (action == GLFW_REPEAT || action == GLFW_PRESS))
    {
        // Zoom out by increasing viewDistance
        viewDistance += 5.0f;
        if (viewDistance > 1000.0f) viewDistance = 1000.0f; // Prevent going too far
        eye_center.y = viewDistance * cos(viewPolar);
        eye_center.x = viewDistance * cos(viewAzimuth);
        eye_center.z = viewDistance * sin(viewAzimuth);
        std::cout << "Zooming out: " << viewDistance << std::endl;
    }

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
} */