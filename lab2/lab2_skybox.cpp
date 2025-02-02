#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

// GLTF model loader
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <render/shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#include <cstdlib> // for rand() and seeding random numbers for building sizes
#include <ctime>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static GLFWwindow *window;
static int windowWidth = 2048;
static int windowHeight = 1536;							
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void processInput();

// OpenGL camera view parameters
//static glm::vec3 eye_center; // WAS WORKING
static glm::vec3 eye_center(80.0f, 80.0f, 20.0f);
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

static float FoV = 55.0f;
static float zNear = 0.1f; 
static float zFar = 1800.0f;

const float cameraSpeed = 0.1f; // Adjust as needed
const float rotationSpeed = 0.005f;

// View control 
static float viewAzimuth = 0.f;
static float viewPolar = 0.f;
static float viewDistance = 500.0f;

// Lighting  
static glm::vec3 lightIntensity(1e6f, 1e6f, 1e6f);;
static glm::vec3 lightPosition(100.0f, 200.0f, 300.0f);

// Shadow mapping
static glm::vec3 lightUp(0, 0, 1);
const int shadowMapWidth = 2048;
const int shadowMapHeight = 1536;
//static int shadowMapWidth = 0;
//static int shadowMapHeight = 0;

// light projections for shadowmap
glm::mat4 lightProjection, lightView, lightSpaceMatrix;

GLuint depthMapFBO;
GLuint depthMap;

// TODO: set these parameters
static float depthFoV = 80.f;
static float depthNear = 5.0f;
static float depthFar = 1500.0f;

// SPHERE
GLuint sphereVAO, sphereVBO, sphereEBO;
GLuint sphereProgramID; // Shader program for the sphere
glm::vec3 sphereLightPos(0.0f, 15.0f, 60.0f);  // Initial position
glm::vec3 sphereLightColor(0.7f, 0.0f, 0.0f);  // Purple light color
float sphereLightIntensity = 2.0f;             // Light intensity



static GLuint LoadTextureTileBox(const char *texture_file_path, GLenum wrapS, GLenum wrapT) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    GLuint texture;
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);  

    // Set the wrapping mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    // Set filtering modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (img) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
		std::cout << "Texture loaded successfully: " << texture_file_path << std::endl;
    } else {
        std::cout << "Failed to load texture " << texture_file_path << std::endl;
    }
    stbi_image_free(img);

    return texture;
}

// Generating my sphere
void generateSphere(float radius, unsigned int latitudeSegments, unsigned int longitudeSegments, 
                    std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    for (unsigned int lat = 0; lat <= latitudeSegments; ++lat) {
        float theta = lat * M_PI / latitudeSegments; // Angle from pole to pole
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (unsigned int lon = 0; lon <= longitudeSegments; ++lon) {
            float phi = lon * 2.0f * M_PI / longitudeSegments; // Angle around the sphere
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            // Compute vertex position
            float x = radius * cosPhi * sinTheta;
            float y = radius * cosTheta;
            float z = radius * sinPhi * sinTheta;

            // Add vertex to the list
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Texture coordinates (optional, use for lighting or texture mapping)
            vertices.push_back((float)lon / longitudeSegments);
            vertices.push_back((float)lat / latitudeSegments);
        }
    }

    // Compute indices for triangle strips
    for (unsigned int lat = 0; lat < latitudeSegments; ++lat) {
        for (unsigned int lon = 0; lon < longitudeSegments; ++lon) {
            unsigned int first = (lat * (longitudeSegments + 1)) + lon;
            unsigned int second = first + longitudeSegments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

void setupSphere(float radius) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    generateSphere(radius, 50, 50, vertices, indices); // 50 segments for latitude and longitude

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // Texture coord
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Load shaders for the sphere
    sphereProgramID = LoadShadersFromFile("../lab2/sphere.vert", "../lab2/sphere.frag");
}

void renderSphere(glm::mat4 vp, glm::vec3 position, glm::vec3 color, float intensity) {
    glUseProgram(sphereProgramID);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
	model = glm::scale(model, glm::vec3(0.4f)); // Uniform scaling

    glm::mat4 mvp = vp * model;

    glUniformMatrix4fv(glGetUniformLocation(sphereProgramID, "MVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniform3fv(glGetUniformLocation(sphereProgramID, "lightColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(sphereProgramID, "intensity"), intensity);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, 50 * 50 * 6, GL_UNSIGNED_INT, 0); // Adjust based on segments
    glBindVertexArray(0);
}

// RAIN 
struct RainParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
	float length;
};

class RainSystem {
private:
    std::vector<RainParticle> particles;
    GLuint VAO, VBO;
	GLuint programID;
    GLuint vpMatrixID;
    const int MAX_PARTICLES = 10000;
    float spawnHeight = 100.0f;
    float spawnArea = 200.0f;  
	float minSpeed = 20.0f;          
    float maxSpeed = 30.0f;       
    
public:
	void initialize() {
        programID = LoadShadersFromFile("../lab2/rain.vert", "../lab2/rain.frag");
        if (programID == 0) {
            std::cerr << "Failed to load rain shaders." << std::endl;
            return;
        }

		vpMatrixID = glGetUniformLocation(programID, "VP");

		for(int i = 0; i < MAX_PARTICLES; i++) {
				RainParticle particle;
				resetParticle(particle, ((rand() % 1000) / 1000.0f) * spawnHeight);
            	particles.push_back(particle);
			}
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

		std::vector<glm::vec3> vertices(MAX_PARTICLES * 2);
        
        //std::vector<glm::vec3> vertices;
        //for(int i = 0; i < MAX_PARTICLES; i++) {
        //    vertices.push_back(particles[i].position);
        //}
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    }
    
    void resetParticle(RainParticle& particle, float startHeight) {
        float x = ((rand() % 1000) / 1000.0f * 2.0f - 1.0f) * spawnArea;
        float z = ((rand() % 1000) / 1000.0f * 2.0f - 1.0f) * spawnArea;
        particle.position = glm::vec3(x, startHeight, z);
        
        // Randomize velocity
        float speed = minSpeed + ((rand() % 1000) / 1000.0f) * (maxSpeed - minSpeed);
        particle.velocity = glm::vec3(
            ((rand() % 100) / 100.0f - 0.5f) * 1.0f,  // Slight x variation
            -speed,                                    // Downward speed
            ((rand() % 100) / 100.0f - 0.5f) * 1.0f   // Slight z variation
        );
        
        // Randomize drop length
        particle.length = 0.5f + ((rand() % 1000) / 1000.0f) * 1.0f; // Length between 0.5 and 1.5 units
        particle.life = 1.0f;
    }
    
    void update(float deltaTime) {
        std::vector<glm::vec3> vertices;
        vertices.reserve(MAX_PARTICLES * 2); // Reserve space for start and end points
        
        for(auto& particle : particles) {
            particle.position += particle.velocity * deltaTime;
            
            if(particle.position.y < 0.0f) {
                resetParticle(particle, spawnHeight);
            }
            
            // Calculate end point of raindrop using velocity direction and length
            glm::vec3 dropDirection = glm::normalize(particle.velocity);
            glm::vec3 endPoint = particle.position + (dropDirection * particle.length);
            
            // Add both vertices for the line
            vertices.push_back(particle.position);
            vertices.push_back(endPoint);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), vertices.data());
    }
    
    void render(const glm::mat4& vp) {
        glUseProgram(programID);
        glUniformMatrix4fv(vpMatrixID, 1, GL_FALSE, glm::value_ptr(vp));
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, MAX_PARTICLES * 2);  // Draw lines instead of points
        
        glDisable(GL_BLEND);
    }

	void cleanup() {
        glDeleteProgram(programID);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }
};

struct Skybox {
	glm::vec3 position;		// Position of the box 
	glm::vec3 scale;		// Size of the box in each axis
	
	GLfloat vertex_buffer_data[72] = {	// Vertex definition for a canonical box
		// Front face
		-1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, 1.0f, 
		1.0f, 1.0f, 1.0f, 
		-1.0f, 1.0f, 1.0f, 
		
		// Back face 
		1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 
		1.0f, 1.0f, -1.0f,
		
		// Left face
		-1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, 1.0f, 
		-1.0f, 1.0f, 1.0f, 
		-1.0f, 1.0f, -1.0f, 

		// Right face 
		1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f, 1.0f, -1.0f, 
		1.0f, 1.0f, 1.0f,

		// Top face
		-1.0f, 1.0f, 1.0f, 
		1.0f, 1.0f, 1.0f, 
		1.0f, 1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 

		// Bottom face
		-1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, 1.0f, 
		-1.0f, -1.0f, 1.0f, 
	};

	GLfloat color_buffer_data[72] = {
		// Front, red
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Back, yellow
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		// Left, green
		0.0f, 1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Right, cyan
		0.0f, 1.0f, 1.0f, 
		0.0f, 1.0f, 1.0f, 
		0.0f, 1.0f, 1.0f, 
		0.0f, 1.0f, 1.0f, 

		// Top, blue
		0.0f, 0.0f, 1.0f, 
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Bottom, magenta
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 
		1.0f, 0.0f, 1.0f, 
		1.0f, 0.0f, 1.0f,  
	};

	GLuint index_buffer_data[36] = {		// 12 triangle faces of a box
		0, 1, 2, 	
		0, 2, 3, 
		
		4, 5, 6, 
		4, 6, 7, 

		8, 9, 10, 
		8, 10, 11, 

		12, 13, 14, 
		12, 14, 15, 

		16, 17, 18, 
		16, 18, 19, 

		20, 21, 22, 
		20, 22, 23, 
	};

    // TODO: Define UV buffer data
    // ---------------------------
	GLfloat uv_buffer_data[48] = {
		// Front Z+
		1.0f, 0.666f,  // Bottom-left
		0.75f,  0.666f,  // Bottom-right
		0.75f,  0.333f,  // Top-right
		1.0f, 0.333f,   // Top-left

		// Back Z-
		0.5f, 0.666f,  // Bottom-left
		0.25f,  0.666f,  // Bottom-right
		0.25f,  0.337f,  // Top-right
		0.5f, 0.337f,   // Top-left

		// Left X+
		0.25f,  0.666f,  // Bottom-left
		0.0f, 0.666f,  // Bottom-right
		0.0f, 0.336f,  // Top-right
		0.25f,  0.336f,   // Top-left

		// Right X-
		0.75f,  0.666f,  // Bottom-left
		0.5f, 0.666f,  // Bottom-right
		0.5f, 0.336f,  // Top-right
		0.75f,  0.336f,   // Top-left

		// Top Y+
		0.26f, 0.0f,    // Bottom-left
		0.5f, 0.0f,    // Bottom-right
		0.5f, 0.337f,  // Top-right
		0.26f, 0.337f,   // Top-left

		// Bottom Y-
		0.25f, 0.666f,  // Bottom-left
		0.5f, 0.666f,  // Bottom-right
		0.5f, 1.0f,    // Top-right
		0.25f, 1.0f     // Top-left

	};

    // ---------------------------
    
	// OpenGL buffers
	GLuint vertexArrayID; 
	GLuint vertexBufferID; 
	GLuint indexBufferID; 
	GLuint colorBufferID;
	GLuint uvBufferID;
	GLuint textureID;

	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint textureSamplerID;
	GLuint programID;

	

	void initialize(glm::vec3 position, glm::vec3 scale) {
		// Define scale of the building geometry
		this->position = position;
		this->scale = scale;

		// I ADDED
		// ---------------
		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data		
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the color data
        // TODO: 
		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

		// TODO: Create a vertex buffer object to store the UV data
		// --------------------------------------------------------
		// Create a vertex buffer object to store the UV data
		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

        // --------------------------------------------------------

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromFile("../lab2/box.vert", "../lab2/box.frag");
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Set fog uniforms
		glm::vec3 fogColor(0.6f, 0.7f, 1.0f); // Gray fog
		float fogStart = 50.0f;
		float fogEnd = 300.0f;


		glUniform3fv(glGetUniformLocation(programID, "fogColor"), 1, glm::value_ptr(fogColor));
		glUniform1f(glGetUniformLocation(programID, "fogStart"), fogStart);
		glUniform1f(glGetUniformLocation(programID, "fogEnd"), fogEnd);

		// Get a handle for our "MVP" uniform
		mvpMatrixID = glGetUniformLocation(programID, "MVP");

        // TODO: Load a texture 
        // --------------------
		textureID = LoadTextureTileBox("../lab2/nightSky2.png", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        // --------------------

        // TODO: Get a handle to texture sampler 
        // -------------------------------------
		// Get a handle for our "textureSampler" uniform
		textureSamplerID = glGetUniformLocation(programID,"textureSampler");
		
        // -------------------------------------
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glm::vec3 cameraPos = -glm::vec3(
        cameraMatrix[3][0] * cameraMatrix[0][0] + cameraMatrix[3][1] * cameraMatrix[0][1] + cameraMatrix[3][2] * cameraMatrix[0][2],
        cameraMatrix[3][0] * cameraMatrix[1][0] + cameraMatrix[3][1] * cameraMatrix[1][1] + cameraMatrix[3][2] * cameraMatrix[1][2],
        cameraMatrix[3][0] * cameraMatrix[2][0] + cameraMatrix[3][1] * cameraMatrix[2][1] + cameraMatrix[3][2] * cameraMatrix[2][2]
    );

		glUniform3fv(glGetUniformLocation(programID, "cameraPosition"), 1, glm::value_ptr(cameraPos));

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// TODO: Model transform 
		// -----------------------
        glm::mat4 modelMatrix = glm::mat4(1.0f);    
		//modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::translate(modelMatrix, eye_center);
        // Scale the box along each axis to make it look like a building
        modelMatrix = glm::scale(modelMatrix, scale);
        // -----------------------

		// Set model-view-projection matrix
		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		// TODO: Enable UV buffer and texture sampler
		// ------------------------------------------
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		// Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0); 


        // ------------------------------------------

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			36,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
        //glDisableVertexAttribArray(2);
	}



	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		//glDeleteBuffers(1, &uvBufferID);
		//glDeleteTextures(1, &textureID);
		glDeleteProgram(programID);
	}
}; 


struct MyModel {

	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint jointMatricesID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;

	// Shadow-related members
    GLuint shadowProgramID;  
    GLuint shadowMatrixID;    
	GLuint modelMatrixID;
	GLuint lightSpaceMatrixID;

	tinygltf::Model model;


	// Each VAO corresponds to each mesh primitive in the GLTF model
	struct PrimitiveObject {
		GLuint vao;
		std::map<int, GLuint> vbos;
	};
	std::vector<PrimitiveObject> primitiveObjects;

	// Skinning 
	struct SkinObject {
		// Transforms the geometry into the space of the respective joint
		std::vector<glm::mat4> inverseBindMatrices;  

		// Transforms the geometry following the movement of the joints
		std::vector<glm::mat4> globalJointTransforms;

		// Combined transforms
		std::vector<glm::mat4> jointMatrices;
	};
	std::vector<SkinObject> skinObjects;

	glm::mat4 getNodeTransform(const tinygltf::Node& node) {
		glm::mat4 transform(1.0f); 

		if (node.matrix.size() == 16) {
			transform = glm::make_mat4(node.matrix.data());
		} else {
			if (node.translation.size() == 3) {
				transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
			}
			if (node.rotation.size() == 4) {
				glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
				transform *= glm::mat4_cast(q);
			}
			if (node.scale.size() == 3) {
				transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
			}
		}
		return transform;
	}

	void computeLocalNodeTransform(const tinygltf::Model& model, 
		int nodeIndex, 
		std::vector<glm::mat4> &localTransforms)
	{
		// TODO: your code here
		const tinygltf::Node& node = model.nodes[nodeIndex];
    	localTransforms[nodeIndex] = getNodeTransform(node);

		for (int childIndex : node.children) {
        	computeLocalNodeTransform(model, childIndex, localTransforms);
    	}
	}

	void computeGlobalNodeTransform(const tinygltf::Model& model, 
		const std::vector<glm::mat4> &localTransforms,
		int nodeIndex, const glm::mat4& parentTransform, 
		std::vector<glm::mat4> &globalTransforms)
	{
		// TODO 2: your code here
		glm::mat4 globalTransform = parentTransform * localTransforms[nodeIndex];
    	globalTransforms[nodeIndex] = globalTransform;

    	const tinygltf::Node& node = model.nodes[nodeIndex];
    	for (int childIndex : node.children) {
        	computeGlobalNodeTransform(model, localTransforms, childIndex, globalTransform, globalTransforms);
    	}
	}  


		std::vector<SkinObject> prepareSkinning(const tinygltf::Model &model) {
		std::vector<SkinObject> skinObjects;

		if (model.skins.empty()) {
    		std::cout << "Model has no skins. Skipping skinning setup." << std::endl;
			return {};
		}


		// In our Blender exporter, the default number of joints that may influence a vertex is set to 4, just for convenient implementation in shaders.
		for (size_t i = 0; i < model.skins.size(); i++) {
			SkinObject skinObject;

			const tinygltf::Skin &skin = model.skins[i];

			// Read inverseBindMatrices
			const tinygltf::Accessor &accessor = model.accessors[skin.inverseBindMatrices];
			assert(accessor.type == TINYGLTF_TYPE_MAT4);
			const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			const float *ptr = reinterpret_cast<const float *>(
            	buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);
			
			skinObject.inverseBindMatrices.resize(accessor.count);
			for (size_t j = 0; j < accessor.count; j++) {
				float m[16];
				memcpy(m, ptr + j * 16, 16 * sizeof(float));
				skinObject.inverseBindMatrices[j] = glm::make_mat4(m);
			}

			assert(skin.joints.size() == accessor.count);

			skinObject.globalJointTransforms.resize(skin.joints.size());
			skinObject.jointMatrices.resize(skin.joints.size());

			// ----------------------------------------------
			// TODO 3: your code here to compute joint matrices
			// ----------------------------------------------

			// Compute local and global transforms for all nodes
			// I ADDED
			std::vector<glm::mat4> localTransforms(model.nodes.size());
			for (size_t i = 0; i < model.nodes.size(); i++) {
				computeLocalNodeTransform(model, i, localTransforms);
			}

			std::vector<glm::mat4> globalTransforms(model.nodes.size(), glm::mat4(1.0f));
			for (size_t i = 0; i < model.nodes.size(); i++) {
				computeGlobalNodeTransform(model, localTransforms, i, glm::mat4(1.0f), globalTransforms);
			}
		// ------------------------------- I ADDED
        	// Compute joint matrices
        for (size_t j = 0; j < skin.joints.size(); j++) {
            int jointIndex = skin.joints[j];
            skinObject.globalJointTransforms[j] = globalTransforms[jointIndex];
            skinObject.jointMatrices[j] = skinObject.globalJointTransforms[j] * skinObject.inverseBindMatrices[j];
        }


			// ----------------------------------------------

			skinObjects.push_back(skinObject);
		}
		return skinObjects;
	}

	int findKeyframeIndex(const std::vector<float>& times, float animationTime) 
	{
		int left = 0;
		int right = times.size() - 1;

		while (left <= right) {
			int mid = (left + right) / 2;

			if (mid + 1 < times.size() && times[mid] <= animationTime && animationTime < times[mid + 1]) {
				return mid;
			}
			else if (times[mid] > animationTime) {
				right = mid - 1;
			}
			else { // animationTime >= times[mid + 1]
				left = mid + 1;
			}
		}

		// Target not found
		return times.size() - 2;
	}

		void updateSkinning(const std::vector<glm::mat4> &nodeTransforms) {

		// -------------------------------------------------
		// TODO: Recompute joint matrices 
		
		std::vector<glm::mat4> globalTransforms;
		globalTransforms.resize(model.nodes.size(), glm::mat4(1.0f));

		// global transforms for the root nodess
		const tinygltf::Scene &scene = model.scenes[model.defaultScene];

		for (const int &rootNodeIndex : scene.nodes) {
        computeGlobalNodeTransform(model, nodeTransforms, rootNodeIndex, glm::mat4(1.0f), globalTransforms);
    }

		// Iterate over each skin object
    for (SkinObject &skinObject : skinObjects) {
        size_t skinIndex = &skinObject - &skinObjects[0]; // Index of the current skin object
        const tinygltf::Skin &currentSkin = model.skins[skinIndex];

        // Update joint matrices for the skin
        for (size_t jointIndex = 0; jointIndex < currentSkin.joints.size(); ++jointIndex) {
            int nodeIndex = currentSkin.joints[jointIndex];
            glm::mat4 jointTransform = globalTransforms[nodeIndex];
            skinObject.jointMatrices[jointIndex] = jointTransform * skinObject.inverseBindMatrices[jointIndex];
        }
    }
		// -------------------------------------------------
	}

		void update(float time) {
		// -------------------------------------------------
        // Prepare transformations for all nodes
        std::vector<glm::mat4> currentNodeTransforms;
        currentNodeTransforms.resize(model.nodes.size(), glm::mat4(1.0f));
        // Update the skinning data
        updateSkinning(currentNodeTransforms);
    }

	bool loadModel(tinygltf::Model &model, const char *filename) {
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		if (!warn.empty()) {
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty()) {
			std::cout << "ERR: " << err << std::endl;
		}

		if (!res)
			std::cout << "Failed to load glTF: " << filename << std::endl;
		else
			std::cout << "Loaded glTF: " << filename << std::endl;

		return res;
	}

	void initializeModel() {
		// Modify your path if needed
		if (!loadModel(model, "../lab2/bot/bot.gltf")) {
			return;
		}

		// Prepare buffers for rendering 
		primitiveObjects = bindModel(model);

		// Prepare joint matrices
		skinObjects = prepareSkinning(model);


		programID = LoadShadersFromFile("../lab2/bot.vert", "../lab2/bot.frag");
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for GLSL variables
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
		jointMatricesID = glGetUniformLocation(programID, "jointMatrices");

		
		
		

	}

		void bindMesh(std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {

		std::map<int, GLuint> vbos;
		for (size_t i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView &bufferView = model.bufferViews[i];

			int target = bufferView.target;
			
			if (bufferView.target == 0) { 
				// The bufferView with target == 0 in our model refers to 
				// the skinning weights, for 25 joints, each 4x4 matrix (16 floats), totaling to 400 floats or 1600 bytes. 
				// So it is considered safe to skip the warning.
				//std::cout << "WARN: bufferView.target is zero" << std::endl;
				continue;
			}

			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(target, vbo);
			glBufferData(target, bufferView.byteLength,
						&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
			
			vbos[i] = vbo;
		}

		// Each mesh can contain several primitives (or parts), each we need to 
		// bind to an OpenGL vertex array object
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			GLuint vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			for (auto &attrib : primitive.attributes) {
				tinygltf::Accessor accessor = model.accessors[attrib.second];
				int byteStride =
					accessor.ByteStride(model.bufferViews[accessor.bufferView]);
				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				if (attrib.first.compare("POSITION") == 0) vaa = 0;
				if (attrib.first.compare("NORMAL") == 0) vaa = 1;
				if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
				if (attrib.first.compare("JOINTS_0") == 0) vaa = 3;
				if (attrib.first.compare("WEIGHTS_0") == 0) vaa = 4;

				if (vaa > -1) {
					glEnableVertexAttribArray(vaa);
					glVertexAttribPointer(vaa, size, accessor.componentType,
										accessor.normalized ? GL_TRUE : GL_FALSE,
										byteStride, BUFFER_OFFSET(accessor.byteOffset));
				} else {
					std::cout << "vaa missing: " << attrib.first << std::endl;
				}
			}

			// Record VAO for later use
			PrimitiveObject primitiveObject;
			primitiveObject.vao = vao;
			primitiveObject.vbos = vbos;
			primitiveObjects.push_back(primitiveObject);

			glBindVertexArray(0);
		}
	}

	void bindModelNodes(std::vector<PrimitiveObject> &primitiveObjects, 
						tinygltf::Model &model,
						tinygltf::Node &node) {

		// Bind buffers for the current mesh at the node
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			bindMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}

		// Recursive into children nodes
		for (size_t i = 0; i < node.children.size(); i++) {
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}

	std::vector<PrimitiveObject> bindModel(tinygltf::Model &model) {
		std::vector<PrimitiveObject> primitiveObjects;

		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}

		return primitiveObjects;
	}

	void drawMesh(const std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {
		
		for (size_t i = 0; i < mesh.primitives.size(); ++i) 
		{
			GLuint vao = primitiveObjects[i].vao;
			std::map<int, GLuint> vbos = primitiveObjects[i].vbos;

			glBindVertexArray(vao);

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

			glDrawElements(primitive.mode, indexAccessor.count,
						indexAccessor.componentType,
						BUFFER_OFFSET(indexAccessor.byteOffset));

			glBindVertexArray(0);
		}
	}

	void drawModelNodes(const std::vector<PrimitiveObject>& primitiveObjects,
						tinygltf::Model &model, tinygltf::Node &node) {
		// Draw the mesh at the node, and recursively do so for children nodes
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			drawMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}
		for (size_t i = 0; i < node.children.size(); i++) {
			drawModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}
	void drawModel(const std::vector<PrimitiveObject>& primitiveObjects,
				tinygltf::Model &model) {
		// Draw all nodes
		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			drawModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}
	}

	
	void cleanup() {
		glDeleteProgram(programID);
	}

};

GLuint groundVAO, groundVBO, groundEBO, groundUVBuffer, NormalBuffer;
GLuint groundTextureID;

	void setupGroundBuffers(GLuint &VAO, GLuint &VBO, GLuint &EBO, GLuint &UVBuffer, GLuint &NormalBuffer) {
    GLfloat groundVertices[] = {
        -300.0f, 0.0f, -300.0f, 
         300.0f, 0.0f, -300.0f, 	// were 500
         300.0f, 0.0f,  300.0f, 
        -300.0f, 0.0f,  300.0f
    };

    GLuint groundIndices[] = {
        0, 1, 2,
        0, 2, 3
    };

    GLfloat groundUVs[] = {
        0.0f, 0.0f, 
        20.0f, 0.0f, 
        20.0f, 20.0f, 
        0.0f, 20.0f
    };

	GLfloat groundNormals[] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};

	//GLfloat groundColors[] = {
	//	1.0f, 1.0f, 1.0f, // White for vertex 0
	//	1.0f, 1.0f, 1.0f, // White for vertex 1
	//	1.0f, 1.0f, 1.0f, // White for vertex 2
	//	1.0f, 1.0f, 1.0f  // White for vertex 3
//};

    // Generate and bind the VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Generate and bind the VBO for vertices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glEnableVertexAttribArray(0); // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Generate and bind the VBO for UVs
    glGenBuffers(1, &UVBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundUVs), groundUVs, GL_STATIC_DRAW);

    // Set UV attribute pointers
    glEnableVertexAttribArray(2); // UV attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Generate and bind the VBO for normals
    glGenBuffers(1, &NormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundNormals), groundNormals, GL_STATIC_DRAW);

    // Set normal attribute pointers
    glEnableVertexAttribArray(1); // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	/*glGenBuffers(1, &colorBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundColors), groundColors, GL_STATIC_DRAW);

	glEnableVertexAttribArray(1); // Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0); */

    // Generate and bind the EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

    // Unbind the VAO to avoid side effects
    glBindVertexArray(0);

    // Also unbind VBO and EBO to clean up the state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}



	void renderGround(glm::mat4 vp, glm::mat4 modelMatrix, GLuint VAO, GLuint textureID, GLuint programID, GLuint mvpMatrixID, GLuint textureSamplerID, glm::vec3 cameraPosition) {
    glUseProgram(programID);

	glUniform3fv(glGetUniformLocation(programID, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
    // Model matrix for the ground
    //glm::mat4 modelMatrix = glm::mat4(1.0f);
    //modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); // Slightly below zero
	glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // Calculate the MVP matrix
    glm::mat4 mvp = vp * modelMatrix;
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
	
    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(textureSamplerID, 0);

    // Bind and draw the geometry
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

	struct ModelInstance {
		glm::vec3 position;
		glm::vec3 scale;
		float rotation;
	};

	std::vector<ModelInstance> modelInstances = {
					// POSITION						// SCALE
		{glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(20.0f, 20.0f, 20.0f), 0.0f},  // Original model
		{glm::vec3(40.0f, 20.0f, 18.0f), glm::vec3(15.0f, 15.0f, 15.0f), 50.0f}, // R1
		{glm::vec3(65.0f, 70.0f, 5.0f), glm::vec3(25.0f, 25.0f, 25.0f), 90.0f},  // R2
		{glm::vec3(-45.0f, 25.0f, 60.0f), glm::vec3(18.0f, 18.0f, 18.0f), 290.0f}, // L1
		{glm::vec3(-55.0f, 80.0f, 5.0f), glm::vec3(22.0f, 22.0f, 22.0f), 250.0f},  // L2
		{glm::vec3(-20.0f, 110.0f, 80.0f), glm::vec3(15.0f, 15.0f, 15.0f), 210.0f}, // L3

		// Far buildings
		{glm::vec3(150.0f, 100.0f, 20.0f), glm::vec3(30.0f, 30.0f, 30.0f), 45.0f},
		{glm::vec3(-200.0f, 150.0f, 5.0f), glm::vec3(25.0f, 25.0f, 25.0f), 30.0f},
		{glm::vec3(180.0f, 60.0f, 5.0f), glm::vec3(20.0f, 20.0f, 20.0f), 60.0f},
		{glm::vec3(-250.0f, 0.0f, 30.0f), glm::vec3(35.0f, 35.0f, 35.0f), 75.0f},
		{glm::vec3(25.0f, 180.0f, 5.0f), glm::vec3(28.0f, 28.0f, 28.0f), 90.0f},
		{glm::vec3(70.0f, 140.0f, 5.0f), glm::vec3(22.0f, 22.0f, 22.0f), 200.0f},
		{glm::vec3(-50.0f, 130.0f, 15.0f), glm::vec3(36.0f, 36.0f, 36.0f), 230.0f},
		{glm::vec3(-25.0f, 190.0f, 10.0f), glm::vec3(23.0f, 23.0f, 23.0f), 180.0f},
		{glm::vec3(-80.0f, 50.0f, 5.0f), glm::vec3(32.0f, 32.0f, 32.0f), 310.0f},
		{glm::vec3(-70.0f, 90.0f, 10.0f), glm::vec3(23.0f, 23.0f, 23.0f), 180.0f},
		{glm::vec3(-50.0f, 130.0f, 15.0f), glm::vec3(36.0f, 36.0f, 36.0f), 230.0f},
		{glm::vec3(-25.0f, -30.0f, 5.0f), glm::vec3(23.0f, 23.0f, 23.0f), 180.0f},

	};


	void renderInstances(glm::mat4 cameraMatrix, const std::vector<ModelInstance>& instances, MyModel& model) {
			glUseProgram(model.programID);

			//glm::vec3 cameraPos = -glm::vec3(
			//cameraMatrix[3][0] * cameraMatrix[0][0] + cameraMatrix[3][1] * cameraMatrix[0][1] + cameraMatrix[3][2] * cameraMatrix[0][2],
			//cameraMatrix[3][0] * cameraMatrix[1][0] + cameraMatrix[3][1] * cameraMatrix[1][1] + cameraMatrix[3][2] * cameraMatrix[1][2],
			//cameraMatrix[3][0] * cameraMatrix[2][0] + cameraMatrix[3][1] * cameraMatrix[2][1] + cameraMatrix[3][2] * cameraMatrix[2][2]
		//);
			glm::vec3 cameraPos = eye_center;
			glUniform3fv(glGetUniformLocation(model.programID, "cameraPosition"), 1, glm::value_ptr(cameraPos));
			glUniform3fv(glGetUniformLocation(model.programID, "viewPos"), 1, glm::value_ptr(cameraPos)); // ADDED NOW
			glUniform3fv(glGetUniformLocation(model.programID, "sphereLightPos"), 1, glm::value_ptr(sphereLightPos));
			glUniform3fv(glGetUniformLocation(model.programID, "sphereLightColor"), 1, glm::value_ptr(sphereLightColor));
			glUniform1f(glGetUniformLocation(model.programID, "sphereLightIntensity"), sphereLightIntensity);

			// Set light data
			glUniform3fv(model.lightPositionID, 1, &lightPosition[0]);
			glUniform3fv(model.lightIntensityID, 1, &lightIntensity[0]);

			for (const auto& instance : instances) {
				// Create a model transformation matrix
				glm::mat4 modelMatrix = glm::mat4(1.0f);
				// rotating because model is rotated wrong direction
				modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate 90 degrees around X-axis
				modelMatrix = glm::translate(modelMatrix, instance.position);
				modelMatrix = glm::scale(modelMatrix, instance.scale);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(instance.rotation), glm::vec3(0.0f, 0.0f, 1.0f));

				
				glUniformMatrix4fv(glGetUniformLocation(model.programID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

				// Calculate the normal matrix for correct lighting
        		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        		glUniformMatrix3fv(glGetUniformLocation(model.programID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

				// Calculate the MVP matrix
				glm::mat4 mvp = cameraMatrix * modelMatrix;

				// Pass the MVP matrix to the shader
				glUniformMatrix4fv(model.mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
				
				// Draw the model
				model.drawModel(model.primitiveObjects, model.model);
			}
		}

		struct Sign {
			glm::vec3 position;  
			glm::vec3 scale;     
			float rotation;

			
			GLuint VAO, VBO, EBO, textureID;
			GLuint programID;          
			GLuint mvpMatrixID;
			GLuint modelMatrixID;
			GLuint viewMatrixID;        
			GLuint textureSamplerID; 
			GLuint projectionMatrixID;

			
			void initialize(glm::vec3 position, glm::vec3 scale, float rotation) {
				this->position = position;
				this->scale = scale;
				this->rotation = rotation;

				GLfloat vertices[] = {
					// positions          // UVs		// Normals
					-0.5f,  0.5f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 
					0.5f,  0.5f, 0.0f,   0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
					0.5f, -0.5f, 0.0f,   0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
					-0.5f, -0.5f, 0.0f,   1.0f, 1.0f,  0.0f, 0.0f, 1.0f
        		};

				GLuint indices[] = {
					0, 1, 2,
					2, 3, 0
				};

			// Generate buffers
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);

			// Vertex buffer
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			// Position attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
			glEnableVertexAttribArray(0);

			// UV attribute
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
			glEnableVertexAttribArray(2);

			// Normals
			// Normal attribute
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
			glEnableVertexAttribArray(1);


			glBindVertexArray(0);

			textureID = LoadTextureTileBox("../lab2/signtext9.png", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

			programID = LoadShadersFromFile("../lab2/sign.vert", "../lab2/sign.frag");
			if (programID == 0) {
            	std::cerr << "Failed to load shaders for the sign." << std::endl;
            	return;
        	}

			// Get uniform locations
			modelMatrixID = glGetUniformLocation(programID, "model");
			viewMatrixID = glGetUniformLocation(programID, "view");
			//textureSamplerID = glGetUniformLocation(programID, "textureSampler");
			projectionMatrixID = glGetUniformLocation(programID, "projection");
			textureSamplerID = glGetUniformLocation(programID, "texture1");	
			}


			void render(const glm::mat4& vpMatrix, float time) {
				
				glUseProgram(programID);

				// oscillation for sign to "bob" up and down
				float oscillation = sin(time * 3.5f) * 0.3f;

				// Model matrix for positioning and scaling
				glm::mat4 modelMatrix  = glm::mat4(1.0f);
				modelMatrix  = glm::translate(modelMatrix , position + glm::vec3(0.0f, oscillation, 0.0f));
				modelMatrix  = glm::scale(modelMatrix , scale);
				modelMatrix  = glm::rotate(modelMatrix , glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

				glm::mat4 mvp = vpMatrix * modelMatrix;

				glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

				// Set uniform values
				glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
				glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glUniformMatrix3fv(glGetUniformLocation(programID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

				// Bind texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textureID);
				glUniform1i(glGetUniformLocation(programID, "texture1"), 0);


				// Bind VAO and draw the quad
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
    }
			// Cleanup resources
			void cleanup() {
				glDeleteBuffers(1, &VBO);
				glDeleteBuffers(1, &EBO);
				glDeleteVertexArrays(1, &VAO);
				glDeleteTextures(1, &textureID);
				glDeleteProgram(programID);
    		}	

		};


int main(void)
{	
	eye_center = glm::vec3(-20.0f, 10.0f, 20.0f);
	lookat = glm::vec3(10.0f, 12.0f, 80.0f);

	glm::vec3 direction = glm::normalize(lookat - eye_center);
    viewPolar = glm::asin(direction.y); // Polar angle from the vertical axis
    viewAzimuth = glm::atan(direction.z, direction.x); 

	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	std::cout << "GLFW initialized successfully." << std::endl;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(2048, 1536, "Lab 2", NULL, NULL);
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

	// Declare modelMatrix at the top of main()
	glm::mat4 modelMatrix = glm::mat4(1.0f);


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
	//glDisable(GL_CULL_FACE);
	//glCullFace(GL_FRONT); // Cull front faces to render inner surfaces
	glCullFace(GL_BACK); // OR FRONT


// Seed the random number generator
//srand(static_cast<unsigned int>(time(0)));

	

    Skybox skybox;
		// Random scale values

    glm::vec3 position(0, 0, 0); // Arrange in a grid
    glm::vec3 scale(600.0f, 600.0f, 400.0f); // Uniform size for skybox
    skybox.initialize(position, scale);

	GLuint groundProgramID = LoadShadersFromFile("../lab2/ground.vert", "../lab2/ground.frag");
	GLuint groundMVPMatID = glGetUniformLocation(groundProgramID, "MVP");
	GLuint groundSamplerID = glGetUniformLocation(groundProgramID, "textureSampler");

	// Set the ground modelMatrix (if needed, you can adjust its position later in the loop)
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); 

	// Compute normalMatrix based on modelMatrix
	
	// RAIN
	RainSystem rainSystem;


	// SIGN
	Sign mySign;

	//glm::mat3 normalMatrix = glm::mat3(1.0f);
	//glUniformMatrix3fv(glGetUniformLocation(groundProgramID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	

    groundTextureID = LoadTextureTileBox("../lab2/ground_text6.jpg", GL_REPEAT, GL_REPEAT);
    setupGroundBuffers(groundVAO, groundVBO, groundEBO, groundUVBuffer, NormalBuffer);
	glBindVertexArray(1);             // Bind VAO
	//glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind EBO
	//glDisableVertexAttribArray(1);    // Disable position attribute
	//glDisableVertexAttribArray(1);    // Disable UV attribute
	GLint currentVAO;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
	std::cout << "After setupGroundBuffers, VAO: " << currentVAO << std::endl;

	setupSphere(10.0f); // sphere radius

	MyModel b;
	b.initializeModel();

	rainSystem.initialize();  // RAIN
	// 0.0f, 0.0f, 5.0f
	mySign.initialize(glm::vec3(25.0f, 15.0f, 105.0f), glm::vec3(35.0f, 15.0f, 15.0f), 45.0f);


	// Camera setup
    //eye_center.y = viewDistance * cos(viewPolar);
    //eye_center.x = viewDistance * cos(viewAzimuth);
    //eye_center.z = viewDistance * sin(viewAzimuth);

	glm::mat4 viewMatrix, projectionMatrix;
    //glm::float32 FoV = 55;
	//glm::float32 zNear = 0.1f; 
	//glm::float32 zFar = 1800.0f;
	projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

	// Time and frame rate tracking
	static double lastTime = glfwGetTime();
	float time = 0.0f;			// Animation time 
	float fTime = 0.0f;			// Time for measuring fps
	unsigned long frames = 0;

	do
	{
		// shadow pass
		//shadowPass(b, windowWidth, windowHeight);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		processInput();

		// Update states for animation
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;

		

		rainSystem.update(deltaTime); // RAIN updating

		viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;

		float time = glfwGetTime();
		float radius = 20.0f;       // Radius of the circular path
    	sphereLightPos.x = 0.0f + radius * cos(time); 
    	sphereLightPos.z = 60.0f + radius * sin(time); 
    	sphereLightPos.y = 15.0f;  

		// Render the skybox first
		glUseProgram(skybox.programID);
		glBindVertexArray(skybox.vertexArrayID); 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skybox.textureID);
		glDepthFunc(GL_LEQUAL); 
		glDisable(GL_CULL_FACE); // Disable culling for the skybox
		skybox.render(vp);

		// Render the ground
		glUseProgram(groundProgramID);

		// Pass sphere light properties
		glUniform3fv(glGetUniformLocation(groundProgramID, "sphereLightPos"), 1, glm::value_ptr(sphereLightPos));
		glUniform3fv(glGetUniformLocation(groundProgramID, "sphereLightColor"), 1, glm::value_ptr(sphereLightColor));
		glUniform1f(glGetUniformLocation(groundProgramID, "sphereLightIntensity"), sphereLightIntensity);

		// Pass view position (camera position)
		glm::vec3 viewPos = eye_center;
		glUniform3fv(glGetUniformLocation(groundProgramID, "viewPos"), 1, glm::value_ptr(viewPos));

		glBindVertexArray(groundVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, groundTextureID);
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); // Adjust position if needed

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix3fv(glGetUniformLocation(groundProgramID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

		renderGround(vp, modelMatrix, groundVAO, groundTextureID, groundProgramID, groundMVPMatID, groundSamplerID, eye_center);
		//renderGround(vp, groundVAO, groundTextureID, groundProgramID, groundMVPMatID, groundSamplerID);
		
		// Render the building
		//b.render(vp);

		renderSphere(vp, sphereLightPos, sphereLightColor, sphereLightIntensity);
		//renderSphere(vp, glm::vec3(0.0f, 15.0f, 60.0f), glm::vec3(0.6f, 0.2f, 0.8f), 0.8f);

		renderInstances(vp, modelInstances, b);

		rainSystem.render(vp);

		glUseProgram(mySign.programID);

		// Pass light and view uniform values once
		glUniform3fv(glGetUniformLocation(mySign.programID, "sphereLightPos"), 1, glm::value_ptr(sphereLightPos));
		glUniform3fv(glGetUniformLocation(mySign.programID, "sphereLightColor"), 1, glm::value_ptr(sphereLightColor));
		glUniform1f(glGetUniformLocation(mySign.programID, "sphereLightIntensity"), sphereLightIntensity);
		glUniform3fv(glGetUniformLocation(mySign.programID, "viewPos"), 1, glm::value_ptr(viewPos));
		mySign.render(vp, glfwGetTime());

				// FPS tracking 
		// Count number of frames over a few seconds and take average
		frames++;
		fTime += deltaTime;
		if (fTime > 2.0f) {		
			float fps = frames / fTime;
			frames = 0;
			fTime = 0;
			
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << "Final Project | Frames per second (FPS): " << fps;
			glfwSetWindowTitle(window, stream.str().c_str());
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	
	// Clean up
	skybox.cleanup();

	b.cleanup();

	rainSystem.cleanup();

	mySign.cleanup();
	
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

bool keys[1024] = { false };

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{

	//static glm::vec3 cameraOffset(0.0f, 0.0f, 0.0f);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_PRESS || action == GLFW_RELEASE)
            keys[key] = false;
    }

  
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        viewAzimuth = 0.f;
        viewPolar = 0.f;
        //cameraOffset = glm::vec3(0.0f, 0.0f, 0.0f); 
        eye_center = glm::vec3(0, 0, 500.0f);
        std::cout << "Reset." << std::endl;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void processInput()
{	
	
	static glm::vec3 cameraOffset(0.0f, 0.0f, 0.0f);
	
    glm::vec3 forward = glm::normalize(glm::vec3(
        cos(viewPolar) * cos(viewAzimuth),
        sin(viewPolar),
        cos(viewPolar) * sin(viewAzimuth)
    ));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    
    glm::vec3 movement(0.0f);
    
    if (keys[GLFW_KEY_W])
        movement += forward;
    if (keys[GLFW_KEY_S])
        movement -= forward;
    if (keys[GLFW_KEY_A])
        movement -= right;
    if (keys[GLFW_KEY_D])
        movement += right;
    
    if (glm::length(movement) > 0.0f)
    {
        movement = glm::normalize(movement) * cameraSpeed;
        cameraOffset += movement;
    }
	
	if (keys[GLFW_KEY_DOWN])
    {
        viewPolar -= rotationSpeed;
        if (viewPolar < -glm::half_pi<float>()) // Prevent flipping over
            viewPolar = -glm::half_pi<float>() + 0.01f;
    }

    if (keys[GLFW_KEY_UP])
    {
        viewPolar += rotationSpeed;
        if (viewPolar > glm::half_pi<float>()) 
            viewPolar = glm::half_pi<float>() - 0.01f;
    }

    if (keys[GLFW_KEY_LEFT])
    {
        viewAzimuth -= rotationSpeed;
        if (viewAzimuth < 0.0f)
            viewAzimuth += glm::two_pi<float>(); 
    }

    if (keys[GLFW_KEY_RIGHT])
    {
        viewAzimuth += rotationSpeed;
        if (viewAzimuth > glm::two_pi<float>())
            viewAzimuth -= glm::two_pi<float>(); 
    }

	 viewPolar = glm::clamp(viewPolar, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);
    if (viewAzimuth < 0.0f)
        viewAzimuth += glm::two_pi<float>();
    else if (viewAzimuth > glm::two_pi<float>())
        viewAzimuth -= glm::two_pi<float>();

    // Update forward vector and lookat
    forward = glm::normalize(glm::vec3(
        cos(viewPolar) * cos(viewAzimuth),
        sin(viewPolar),
        cos(viewPolar) * sin(viewAzimuth)
    ));
    

    // Update camera position
	cameraOffset = movement;
    eye_center += cameraOffset;
	eye_center.y = 10.0f;
    //lookat = eye_center + forward * viewDistance;
	lookat = eye_center + forward;
}