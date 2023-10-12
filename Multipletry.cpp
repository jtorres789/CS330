#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

#include <iostream>

#include "cylinder.h"

#include "camera.h"

//#include "plane.h"

using namespace std;

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera gCamera(glm::vec3(0.0f, 3.0f, 7.0f));
float gLastX = SCR_WIDTH / 2.0f;
float gLastY = SCR_HEIGHT / 2.0f;
bool gFirstMouse = true;

bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;


//shaderprogram
GLuint gCubeProgramId;
GLuint gLamp1ProgramId;

// Pyramid and light color
   //m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
glm::vec3 gObjectColor(0.2f, 0.3f, 0.3f);
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

// Light position and scale
glm::vec3 gLight1Position(-1.5f, 3.5f, 3.0f);
glm::vec3 gLight1Scale(2.0f);
glm::vec2 gUVScale(5.0f, 5.0f);

// timing
float gDeltaTime = 0.0f;	// time between current frame and last frame
float gLastFrame = 0.0f;

bool orthographic = false;

const GLchar* PyramidFragmentShaderSource = GLSL(440,

	in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void mainphong()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components for number one pyramid*/

	//Calculate Ambient lighting*/
	float ambientStrength = 0.50f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse = impact * lightColor; // Generate diffuse light color

	//Calculate Specular lighting*/
	float specularIntensity = 0.8f; // Set specular light strength
	float highlightSize = 100.0f; // Set specular highlight size
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.5), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;

	// Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	// Calculate phong result
	vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

	fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Lamp1 Shader Source Code*/
const GLchar* lamp1VertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

/* Fragment Shader1  Source Code*/
const GLchar* lamp1FragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(0.2f, 0.3f, 0.3f, 1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Multiple Object Try", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, UMousePositionCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------
	Shader ourShader("shaderfiles/7.3.camera.vs", "shaderfiles/7.3.camera.fs");
	Shader lightingShader("shaderfiles//6.multiple_lights.vs", "shaderfiles//6.multiple_lights.fs");
	Shader lightCubeShader("shaderfiles//6.light_cube.vs", "shaderfiles//6.light_cube.fs");

	// Can head cube 
	// ------------------------------------------------------------------
	float vertices[] = {

		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f



		
	};

	float vertices2[] = {

		//PLane or table
		//top table
		-5.0f, -2.5f, -5.0f,  0.0f, 0.0f,
		 5.0f, -2.5f, -5.0f,  0.0f, 1.0f, 
		 5.0f, -2.5f,  5.0f,  1.0f, 1.0f,

		 5.0f, -2.5f,  5.0f,  0.0f, 0.0f, 
		-5.0f, -2.5f,  5.0f,  0.0f, 1.0f, 
		-5.0f, -2.5f, -5.0f,  1.0f, 1.0f,
		//bottom table
		- 5.0f, -3.0f, -5.0f,  0.0f, 0.0f,
		 5.0f, -3.0f, -5.0f,  0.0f, 1.0f,
		 5.0f, -3.0f,  5.0f,  1.0f, 1.0f,

		 5.0f, -3.0f,  5.0f,  0.0f, 0.0f,
		-5.0f, -3.0f,  5.0f,  0.0f, 1.0f,
		-5.0f, -3.0f, -5.0f,  1.0f, 1.0f,

		//front side
		 5.0f, -3.0f,  5.0f,  0.0f, 0.0f,
		-5.0f, -3.0f,  5.0f,  0.0f, 1.0f,
		-5.0f, -2.5f,  5.0f,  1.0f, 1.0f,

		-5.0f, -2.5f,  5.0f,  0.0f, 0.0f,
		 5.0f, -3.0f,  5.0f,  0.0f, 1.0f,
		 5.0f, -2.5f,  5.0f,  1.0f, 1.0f,

		 //back side
		-5.0f, -2.5f,  -5.0f,  0.0f, 0.0f,
		 5.0f, -3.0f,  -5.0f,  0.0f, 1.0f,
		 5.0f, -2.5f,  -5.0f,  1.0f, 1.0f,

		 5.0f, -3.0f,  -5.0f,  0.0f, 0.0f,
		-5.0f, -3.0f,  -5.0f,  0.0f, 1.0f,
		-5.0f, -2.5f,  -5.0f,  1.0f, 1.0f,


		//SIDE 1
		-5.0f, -2.5f,   5.0f,  0.0f, 0.0f,
		-5.0f, -3.0f,  -5.0f,  0.0f, 1.0f,
		-5.0f, -2.5f,  -5.0f,  1.0f, 1.0f,

		-5.0f, -3.0f,  -5.0f,  0.0f, 0.0f,
		-5.0f, -3.0f,   5.0f,  0.0f, 1.0f,
		-5.0f, -2.5f,   5.0f,  1.0f, 1.0f,

		//SIDE 2
		5.0f, -2.5f,   5.0f,  0.0f, 0.0f,
		5.0f, -3.0f,  -5.0f,  0.0f, 1.0f,
		5.0f, -2.5f,  -5.0f,  1.0f, 1.0f,

		5.0f, -3.0f,  -5.0f,  0.0f, 0.0f,
		5.0f, -3.0f,   5.0f,  0.0f, 1.0f,
		5.0f, -2.5f,   5.0f,  1.0f, 1.0f,


	};

	float vertices3[] = {
		
		//RED TRIANGLE PIECE

		//TOP
		 0.1f, -3.1f, -3.8f,  0.0f, 0.0f,
		 2.0f, -3.1f, -5.0f,  0.0f, 1.0f,
		 2.0f, -3.1f, -3.0f,  1.0f, 1.0f,

		 //BOTTOM
		 0.1f, -3.18f, -3.8f,  0.0f, 0.0f,
		 2.0f, -3.18f, -5.0f,  0.0f, 1.0f,
		 2.0f, -3.18f, -3.0f,  1.0f, 1.0f,

		 //SIDE1
		2.0f, -3.1f, -3.0f,  0.0f, 0.0f,
		2.0f, -3.18f, -5.0f,  0.0f, 1.0f,
		2.0f, -3.18f, -3.0f,  1.0f, 1.0f,

	   2.0f,  -3.1f, -5.0f,  0.0f, 0.0f,
	   2.0f, -3.1f, -3.0f,  0.0f, 1.0f,
	   2.0f, -3.18f, -5.0f,  1.0f, 1.0f,

	   //SIDE2
	   2.0f, -3.1f, -3.0f,  0.0f, 0.0f,
	   0.1f, -3.18f, -3.8f,  0.0f, 1.0f,
	   2.0f, -3.18f, -3.0f,  1.0f, 1.0f,

	   0.1f,  -3.1f, -3.8f,  0.0f, 0.0f,
	   2.0f, -3.1f, -3.0f,  0.0f, 1.0f,
	   0.1f, -3.18f, -3.8f,  1.0f, 1.0f,
	};

	float vertices4[] = {
		//top table
		-1.0f, -2.5f, -1.0f,  0.0f, 0.0f,
		 1.0f, -2.5f, -1.0f,  0.0f, 1.0f,
		 1.0f, -2.5f,  1.0f,  1.0f, 1.0f,

		 1.0f, -2.5f,  1.0f,  0.0f, 0.0f,
		-1.0f, -2.5f,  1.0f,  0.0f, 1.0f,
		-1.0f, -2.5f, -1.0f,  1.0f, 1.0f,
		//bottom table
		-1.0f, -3.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -3.0f, -1.0f,  0.0f, 1.0f,
		 1.0f, -3.0f,  1.0f,  1.0f, 1.0f,

		 1.0f, -3.0f,  1.0f,  0.0f, 0.0f,
		-1.0f, -3.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -3.0f, -1.0f,  1.0f, 1.0f,

		//front side
		 1.0f, -3.0f,  1.0f,  1.0f, 0.0f,
		-1.0f, -3.0f,  1.0f,  1.0f, 1.0f,
		-1.0f, -2.5f,  1.0f,  1.0f, 1.0f,

		-1.0f, -2.5f,  1.0f,  1.0f, 0.0f,
		 1.0f, -3.0f,  1.0f,  1.0f, 1.0f,
		 1.0f, -2.5f,  1.0f,  1.0f, 1.0f,

		 //back side
		-1.0f, -2.5f,  -1.0f,  1.0f, 0.0f,
		 1.0f, -3.0f,  -1.0f,  1.0f, 1.0f,
		 1.0f, -2.5f,  -1.0f,  1.0f, 1.0f,

		 1.0f, -3.0f,  -1.0f,  1.0f, 0.0f,
		-1.0f, -3.0f,  -1.0f,  1.0f, 1.0f,
		-1.0f, -2.5f,  -1.0f,  1.0f, 1.0f,


		//SIDE 1
		-1.0f, -2.5f,   1.0f,  1.0f, 0.0f,
		-1.0f, -3.0f,  -1.0f,  1.0f, 1.0f,
		-1.0f, -2.5f,  -1.0f,  1.0f, 1.0f,

		-1.0f, -3.0f,  -1.0f,  1.0f, 0.0f,
		-1.0f, -3.0f,   1.0f,  1.0f, 1.0f,
		-1.0f, -2.5f,   1.0f,  1.0f, 1.0f,

		//SIDE 2
		1.0f, -2.5f,   1.0f,  1.0f, 0.0f,
		1.0f, -3.0f,  -1.0f,  1.0f, 1.0f,
		1.0f, -2.5f,  -1.0f,  1.0f, 1.0f,

		1.0f, -3.0f,  -1.0f,  1.0f, 0.0f,
		1.0f, -3.0f,   1.0f,  1.0f, 1.0f,
		1.0f, -2.5f,   1.0f,  1.0f, 1.0f,

	};

	unsigned int VBO, VAO;
	unsigned int VBO2, VAO2;
	unsigned int VBO3, VAO3;
	unsigned int VBO4, VAO4;
	unsigned int VBO5, VAO5;
	unsigned int VBO6, VAO6;
	unsigned int VBO7, VAO7;
	unsigned int VBO8, VAO8;
	unsigned int VBO9, VAO9;
	unsigned int lightCubeVAO;
	unsigned int lightCubeVAO2;

	glGenVertexArrays(1, &lightCubeVAO);
	glBindVertexArray(lightCubeVAO);

	glGenVertexArrays(1, &lightCubeVAO2);
	glBindVertexArray(lightCubeVAO2);


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	glGenVertexArrays(1, &VAO2);
	glGenBuffers(1, &VBO2);
	glBindVertexArray(VAO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);

	glGenVertexArrays(1, &VAO3);
	glGenBuffers(1, &VBO3);
	glBindVertexArray(VAO3);
	glBindBuffer(GL_ARRAY_BUFFER, VBO3);


	glGenVertexArrays(1, &VAO4);
	glGenBuffers(1, &VBO4);
	glBindVertexArray(VAO4);
	glBindBuffer(GL_ARRAY_BUFFER, VBO4);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);


	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	glGenVertexArrays(1, &VAO5);
	glGenBuffers(1, &VBO5);
	glBindVertexArray(VAO5);
	glBindBuffer(GL_ARRAY_BUFFER, VBO5);

	glGenVertexArrays(1, &VAO6);
	glGenBuffers(1, &VBO6);
	glBindVertexArray(VAO6);
	glBindBuffer(GL_ARRAY_BUFFER, VBO6);

	glGenVertexArrays(1, &VAO7);
	glGenBuffers(1, &VBO7);
	glBindVertexArray(VAO7);
	glBindBuffer(GL_ARRAY_BUFFER, VBO7);

	glGenVertexArrays(1, &VAO8);
	glGenBuffers(1, &VBO8);
	glBindVertexArray(VAO8);
	glBindBuffer(GL_ARRAY_BUFFER, VBO8);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), vertices3, GL_STATIC_DRAW);


	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);




	glGenVertexArrays(1, &VAO9);
	glGenBuffers(1, &VBO9);
	glBindVertexArray(VAO9);
	glBindBuffer(GL_ARRAY_BUFFER, VBO9);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices4), vertices4, GL_STATIC_DRAW);


	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);




	// load and create a texture 
	// -----------------------
	unsigned int texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10;
	// texture 1
	// ---------
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load("images/BLACK.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	// texture 2
	// ---------
	glGenTextures(1, &texture2);
	glBindTexture(GL_TEXTURE_2D, texture2);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width1, height1, nrChannels1;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data4 = stbi_load("images/spoon_head.jpg", &width1, &height1, &nrChannels1, 0);
	if (data)
	{
		// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data4);
		//glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data4);

	// texture 3
// ---------
	glGenTextures(1, &texture3);
	glBindTexture(GL_TEXTURE_2D, texture3);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data2 = stbi_load("images/steel.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data2);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data2);


	//set texture 4
	glGenTextures(1, &texture4);
	glBindTexture(GL_TEXTURE_2D, texture4);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data3 = stbi_load("images/sheetmetal.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data3);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data3);

	//set texture 5
	glGenTextures(1, &texture5);
	glBindTexture(GL_TEXTURE_2D, texture5);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data5 = stbi_load("images/wood.jpeg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data5);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data5);

	//set texture 6
	glGenTextures(1, &texture6);
	glBindTexture(GL_TEXTURE_2D, texture6);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data6 = stbi_load("images/pencil.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data6);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data6);

	//set texture 7
	glGenTextures(1, &texture7);
	glBindTexture(GL_TEXTURE_2D, texture7);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data7 = stbi_load("images/pencil2metal.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data7);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data7);

	//set texture 8
	glGenTextures(1, &texture8);
	glBindTexture(GL_TEXTURE_2D, texture8);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data8 = stbi_load("images/pencil2.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data8);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data8);

	//set texture 9
	glGenTextures(1, &texture9);
	glBindTexture(GL_TEXTURE_2D, texture9);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data9 = stbi_load("images/red.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data9);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data9);

	//set texture 9
	glGenTextures(1, &texture10);
	glBindTexture(GL_TEXTURE_2D, texture10);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data10 = stbi_load("images/frame2.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data10);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data10);

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	ourShader.use();
	ourShader.setInt("texture1", 0);
	ourShader.setInt("texture2", 1);
	ourShader.setInt("texture3", 2);
	ourShader.setInt("texture4", 3);
	ourShader.setInt("texture5", 4);
	ourShader.setInt("texture6", 5);
	ourShader.setInt("texture7", 6);
	ourShader.setInt("texture8", 7);
	ourShader.setInt("texture9", 8);
	ourShader.setInt("texture10", 9);


	glm::mat4 model;
	float angle;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, texture2);

		// activate shader
		ourShader.use();

		glm::mat4 projection;
		ourShader.setMat4("projection", projection);
		if (orthographic) {
			float scale = 100;
			projection = glm::ortho(-(800.0f / scale), 800.0f / scale, 600.0f / scale,  -(600.0f / scale), -4.5f, 6.5f);
		}
		else {
			projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);
		}


		// camera/view transformation
		glm::mat4 view = gCamera.GetViewMatrix();
		ourShader.setMat4("view", view);



		// render Air can Cube head
		glBindVertexArray(VAO);
		// calculate the model matrix for each object and pass it to shader before drawing
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(2.0f, 5.0f, 0.0f));
		angle = 0.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(45.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		//can body cylinder
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture3);
		glBindVertexArray(VAO2);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(2.0f, 3.5f, -0.0f));
		angle = 0.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		static_meshes_3D::Cylinder C(0.8, 360, 2, true, true, true);
		C.render();

		//can red straw cylinder
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture4);
		glBindVertexArray(VAO3);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(2.0f, 5.0f, 1.5f));
		angle = 90.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(5.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		static_meshes_3D::Cylinder C2(0.05, 360, 3, true, true, true);
		C2.render();

		//plane 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture5);
		glBindVertexArray(VAO4);
		// calculate the model matrix for each object and pass it to shader before drawing
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
		angle = 0.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
		//Pencil
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture6);
		glBindVertexArray(VAO5);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(-0.0f, 2.55f, 1.5f));
		angle = 90.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(180.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		static_meshes_3D::Cylinder C3(0.05, 6, 3, true, true, true);
		C3.render();
		//Pencil metal end 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture7);
		glBindVertexArray(VAO6);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(-0.0f, 2.55f,  -0.05f));
		angle = 90.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(180.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		static_meshes_3D::Cylinder C4(0.05, 6, 0.09, true, true, true);
		C4.render();
		//pencil eraser 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture8);
		glBindVertexArray(VAO7);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(-0.0f, 2.55f, -0.10f));
		angle = 90.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(180.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		static_meshes_3D::Cylinder C5(0.05, 6, 0.09, true, true, true);
		C5.render();
		//redTriangle on table 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture9);
		glBindVertexArray(VAO8);
		// calculate the model matrix for each object and pass it to shader before drawing
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(-0.0f, 5.7f, 7.0f));
		angle = 0.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		//photoframe 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture10);
		glBindVertexArray(VAO9);
		// calculate the model matrix for each object and pass it to shader before drawing
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(-2.5f, 5.2f, 2.0f));
		angle = 0.0f;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1558.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);

		//draw lamnp
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", view);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3(-5.0f, 10.7f, 7.0f));
		model = glm::scale(model, glm::vec3(0.2f));
		float angle = 45;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		lightCubeShader.setMat4("model", model);
		
		static_meshes_3D::Cylinder C6(15, 360, 0.09, true, true, true);
		C6.render();

		glBindVertexArray(lightCubeVAO);

		//draw lamnp2
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", view);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
		model = glm::translate(model, glm::vec3( 7.0f, 10.7f, 3.0f));
		model = glm::scale(model, glm::vec3(0.2f));
		float angle2 = 45;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		lightCubeShader.setMat4("model", model);

		static_meshes_3D::Cylinder C7(15, 360, 0.09, true, true, true);
		C7.render();

		glBindVertexArray(lightCubeVAO2);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glDeleteVertexArrays(1, &VAO2);
	glDeleteBuffers(1, &VBO2);

	glDeleteVertexArrays(1, &VAO3);
	glDeleteBuffers(1, &VBO3);

	glDeleteVertexArrays(1, &VAO4);
	glDeleteBuffers(1, &VBO4);

	glDeleteVertexArrays(1, &VAO5);
	glDeleteBuffers(1, &VBO5);

	glDeleteVertexArrays(1, &VAO6);
	glDeleteBuffers(1, &VBO6);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // goes up the camera
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // goes down the camera 
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) // P sswitch for ortho and perspective 
		orthographic = !orthographic;


	
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
	{
		if (gFirstMouse)
		{
			gLastX = xpos;
			gLastY = ypos;
			gFirstMouse = false;
		}

		float xoffset = xpos - gLastX;
		float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

		gLastX = xpos;
		gLastY = ypos;

		gCamera.ProcessMouseMovement(xoffset, yoffset);
	}
};
	// glfw: whenever the mouse scroll wheel scrolls, this callback is called
	// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}

void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}
