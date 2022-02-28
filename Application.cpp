#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h> 
#include "camera.h"// GLFW library

#define STB_IMAGE_IMPLEMENTATION
#include <GL/stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Tutorial 4.3"; // Macro for window title

    // Variables for window width and height
    int WINDOW_WIDTH = 800;
    int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint VAO[5];         // Handle for the vertex array object
        GLuint VBO[10];         // Handle for the vertex buffer object
        GLuint nMonumentIndices;    // Number of indices of the mesh
        GLuint nPlaneIndices;
        GLuint nBuildingIndices;
        GLuint nColumnIndices;
        GLuint nPoolIndices;
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Shader programs
    GLuint gSunProgramId;
    GLuint gSpotProgramId;


    glm::vec2 gUVScale(2.0f, 2.0f); //tex scale

    //Textures
    const char* texFilename;
    GLuint marbleTex, grassTex, waterTex, monumentTex;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool ortho = false;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // lighting global variables
    //--------------------------
    // light 1
    glm::vec3 gLightPosition1(10.0f, 0.0f, -20.0f);
    glm::vec3 gLightColor1(1.0f, 1.0f, 1.0f);
    GLfloat light_1_strength = 2.0f;
    // light 2
    glm::vec3 gLightPosition2(0.0f, 10.0f, 20.0f);
    glm::vec3 gLightColor2(0.992f, 0.9843f, 0.8275f);
    GLfloat light_2_strength = 1.0f;
    // base object color
    //glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    // light components
    glm::vec3 gAmbientStrength(glm::vec3(0.15f));
    glm::vec3 gDiffuseStrength(glm::vec3(1.0f));
    float gSpecularIntensity = 0.8;

    bool gIsLampOrbiting = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);


const GLchar* sunVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* sunFragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 lightPos1;
uniform vec3 lightColor1;
uniform float light_1_strength;
uniform vec3 lightPos2;
uniform vec3 lightColor2;
uniform float light_2_strength;
uniform vec3 ambientStrength;
uniform float specularIntensity;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform sampler2D uTextureExtra;
uniform bool multipleTextures;
uniform vec2 uvScale;
//uniform vec3 objectColor;

void main()
{
    // Texture holds the color to be used for all three components of Phong lighting model
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
    // if there is a second image
    if (multipleTextures) {
        // find the color of the second texture based on this fragment's tex coord 
        vec4 extraTexture = texture(uTextureExtra, vertexTextureCoordinate);
        // if this location is not fully transparent, use its color
        if (extraTexture.a != 0.0) {
            textureColor = extraTexture;
        }
    }

    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    // FIRST LIGHT:
    //-------------
    //Calculate Ambient lighting*/
    vec3 ambient = ambientStrength * lightColor1; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos1 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor1; // Generate diffuse light color

    //Calculate Specular lighting*/
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor1;

    // SECOND LIGHT:
    //--------------
    // ambient lighting - add first and second light ambient numbers
    ambient += light_2_strength * (ambientStrength * lightColor2);

    // diffuse lighting
    lightDirection = normalize(lightPos2 - vertexFragmentPos);
    impact = max(dot(norm, lightDirection), 0.0);
    // add first and second light diffuses
    diffuse += light_2_strength * (impact * lightColor2);

    // specular lighting
    reflectDir = reflect(-lightDirection, norm);
    specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    // add first and second light speculars
    specular += light_2_strength * (specularIntensity * specularComponent * lightColor2);

    // CALCULATE PHONG RESULT
    //-----------------------
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

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


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(sunVertexShaderSource, sunFragmentShaderSource, gSunProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gSpotProgramId))
        return EXIT_FAILURE;

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    texFilename = "res/marble.png";
    if (!UCreateTexture(texFilename, marbleTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    else {
        cout << "Texture created successfully" << endl;
    }

    texFilename = "res/grass.jpg";
    if (!UCreateTexture(texFilename, grassTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    else {
        cout << "Texture created successfully" << endl;
    }

    texFilename = "res/water.png";
    if (!UCreateTexture(texFilename, waterTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    else {
        cout << "Texture created successfully" << endl;
    }

    texFilename = "res/offwhite.jpg";
    if (!UCreateTexture(texFilename, monumentTex))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    else {
        cout << "Texture created successfully" << endl;
    }


    // Tell OpenGL for each sampler which texture unit it belongs to (only has to be done once).
    glUseProgram(gSunProgramId);
    // We set the texture as texture unit 0.
    glUniform1i(glGetUniformLocation(gSunProgramId, "marbleTex"), 0);
    glUniform1i(glGetUniformLocation(gSunProgramId, "grassTex"), 1);
    glUniform1i(glGetUniformLocation(gSunProgramId, "waterTex"), 2);
    glUniform1i(glGetUniformLocation(gSunProgramId, "monumentTex"), 3);



    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release shader program
    UDestroyShaderProgram(gSunProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
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
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        ortho = !ortho;

    static bool isLKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;
   
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // Reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
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


// Function called to render a frame
void URender()
{
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition1, 1.0f);
       gLightPosition1.x = newPosition.x;
       gLightPosition1.y = newPosition.y;
       gLightPosition1.z = newPosition.z;
       // gLightPosition2.x = newPosition.x;
       // gLightPosition2.y = newPosition.y;
       // gLightPosition2.z = newPosition.z;

    }


    glEnable(GL_DEPTH_TEST);  //checks to make sure a fragment is supposed to be rendered (front) or not (behind other rendered fragments)

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  //depth buffer stores depth value generated after depth testing



    glm::mat4 scale = glm::scale(glm::vec3(-1.0f, -1.0f, -1.0f)); // scaling the matrix

    glm::mat4 rotation = glm::rotate(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));  //first rotation around the x axis

    glm::mat4 rotation2 = glm::rotate(45.0f, glm::vec3(0.0f, 0.0f, 1.0f));  //second rotation around the z axis

    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));	//no translation	

    glm::mat4 model = translation * rotation * rotation2 * scale;  //combination the matrices to form the model


    glm::mat4 view = gCamera.GetViewMatrix();

    glm::mat4 projection;
    if (!ortho) {
        GLfloat oWidth = (GLfloat)WINDOW_WIDTH * 0.01f; // 10% of width
        GLfloat oHeight = (GLfloat)WINDOW_HEIGHT * 0.01f; // 10% of height
        //projection = glm::ortho(-oWidth, oWidth, oHeight, -oHeight, 0.1f, 100.0f);
        float scale = 100;
        projection = glm::ortho(-((float)WINDOW_WIDTH / scale), (float)WINDOW_WIDTH / scale, -(float)WINDOW_HEIGHT / scale, ((float)WINDOW_HEIGHT / scale), -50.0f, 50.0f);
    }
    else {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // Set the shader to be used
    glUseProgram(gSunProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gSunProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gSunProgramId, "view");
    GLint projLoc = glGetUniformLocation(gSunProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLint UVScaleLoc = glGetUniformLocation(gSunProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Reference matrix uniforms from the shader program for the color, light color, light position, and camera position
    GLint lightColor1Loc = glGetUniformLocation(gSunProgramId, "lightColor1");
    GLint lightColor2Loc = glGetUniformLocation(gSunProgramId, "lightColor2");
    GLint lightPosition1Loc = glGetUniformLocation(gSunProgramId, "lightPos1");
    GLint lightPosition2Loc = glGetUniformLocation(gSunProgramId, "lightPos2");
    GLint lightStrength1Loc = glGetUniformLocation(gSunProgramId, "light_1_strength");
    GLint lightStrength2Loc = glGetUniformLocation(gSunProgramId, "light_2_strength");
    GLint viewPositionLoc = glGetUniformLocation(gSunProgramId, "viewPosition");
    GLint ambientStrengthLoc = glGetUniformLocation(gSunProgramId, "ambientStrength");
    GLint diffuseStrengthLoc = glGetUniformLocation(gSunProgramId, "diffuseStrength");
    GLint specularIntensityLoc = glGetUniformLocation(gSunProgramId, "specularIntensity");

    // Pass color, light, and camera data to the shader program's corresponding uniforms
    glUniform3f(lightColor1Loc, gLightColor1.r, gLightColor1.g, gLightColor1.b);
    glUniform3f(lightColor2Loc, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPosition1Loc, gLightPosition1.x, gLightPosition1.y, gLightPosition1.z);
    glUniform3f(lightPosition2Loc, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    glUniform1f(lightStrength1Loc, light_1_strength);
    glUniform1f(lightStrength2Loc, light_2_strength);
    glUniform3f(ambientStrengthLoc, gAmbientStrength.r, gAmbientStrength.g, gAmbientStrength.b);
    glUniform3f(diffuseStrengthLoc, gDiffuseStrength.r, gAmbientStrength.g, gAmbientStrength.b);
    glUniform1f(specularIntensityLoc, gSpecularIntensity);;
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // tell fragment shader there is not multiple textures
    GLuint multipleTexturesLoc = glGetUniformLocation(gSunProgramId, "multipleTextures");
    glUniform1i(multipleTexturesLoc, false);

    glBindTexture(GL_TEXTURE_2D, monumentTex);
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.VAO[0]);
    glDrawElements(GL_TRIANGLES, gMesh.nMonumentIndices, GL_UNSIGNED_SHORT, NULL);


    glBindTexture(GL_TEXTURE_2D, grassTex);
    glBindVertexArray(gMesh.VAO[1]);
    glDrawElements(GL_TRIANGLES, gMesh.nPlaneIndices, GL_UNSIGNED_SHORT, NULL);


    glBindTexture(GL_TEXTURE_2D, marbleTex);
    glBindVertexArray(gMesh.VAO[2]);
    glDrawElements(GL_TRIANGLES, gMesh.nBuildingIndices, GL_UNSIGNED_SHORT, NULL);


    glBindTexture(GL_TEXTURE_2D, marbleTex);
    glBindVertexArray(gMesh.VAO[3]);
    glDrawElements(GL_TRIANGLES, gMesh.nColumnIndices, GL_UNSIGNED_SHORT, NULL);


    glBindTexture(GL_TEXTURE_2D, waterTex);
    glBindVertexArray(gMesh.VAO[4]);
    glDrawElements(GL_TRIANGLES, gMesh.nPoolIndices, GL_UNSIGNED_SHORT, NULL);



    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    GLfloat monumentVerts[] = {

        //monument
        // Vertex Positions    /0 Colors (r,g,b,a)
        2.5f, 7.5f, 0.0f,	 0.0f, 0.0f, 1.0f,       1.0f, 1.0f,// 0
       -2.5f, 7.5f, 0.0f,	 0.0f, 0.0f, 1.0f,       1.0f, 0.0f,// 1
        2.5f, 12.5f, 0.0f,	 0.0f, 0.0f, 1.0f,       0.0f, 0.0f,// 2
       -2.5f, 12.5f, 0.0f,	 0.0f, 0.0f, 1.0f,       0.0f, 1.0f,// 3
        2.0f, 9.0f, 10.0f,	 0.0f, 0.0f, 1.0f,       1.0f, 1.0f,//  4  
       -2.0f, 9.0f, 10.0f,	 0.0f, 0.0f, 1.0f,       1.0f, 0.0f,//  5
        2.0f, 11.0f, 10.0f,	 0.0f, 0.0f, 1.0f,       0.0f, 0.0f,// 6
       -2.0f, 11.0f, 10.0f,	 0.0f, 0.0f, 1.0f,       0.0f, 1.0f,// 7
        0.0f, 10.0f, 12.0f,  0.0f, 0.0f, 1.0f,        0.0f, 0.0f,// 8

    };

    GLfloat planeVerts[] = {
        -15.0f, 15.0f, 0.0f,    0.0f, 0.0f, -1.0f,  1.0f, 1.0f,// 0
        -15.0f, -15.0f, 0.0f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f,// 1
        15.0f, 15.0f, 0.0f,     0.0f, 0.0f, -1.0f,  0.0f, 0.0f,// 2
        15.0f, -15.0f, 0.0f,    0.0f, 0.0f, -1.0f,  0.0f, 1.0f,// 3
    };

    GLfloat buildingVerts[] = {
        //building base
        1.5f, -11.0f, 0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//0
        1.5f, -10.0f, 0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//1
        -1.5f, -10.0f, 0.0f,0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//2
        -1.5f, -11.0f, 0.0f,0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//3
        1.5f, -11.0f, 0.2f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//4
        1.5f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//5
        -1.5f, -10.0f, 0.2f,0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//6
        -1.5f, -11.0f, 0.2f,0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//7
                                                        //8
                                                        //9
        //building top                                  //10
        1.5f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//11
        1.5f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//12
        -1.5f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//13
        -1.5f, -11.0f, 2.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//14
        1.5f, -11.0f, 2.2f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//15
        1.5f, -10.0f, 2.2f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//16
        -1.5f, -10.0f, 2.2f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//17
        -1.5f, -11.0f, 2.2f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//18
                                                        //19
        //building center                               //20
        0.7f, -10.25f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//21
        0.7f, -10.75f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//22
        -0.7f, -10.25f, 0.2f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//23
        -0.7f, -10.75f, 0.2f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//24
        0.7f, -10.25f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//25
        0.7f, -10.75f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//26
        -0.7f, -10.25f, 2.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//27
        -0.7f, -10.75f, 2.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//28

    };

    GLfloat columnVerts[] =
    {
        //column1
        -1.5f, -11.0f, 0.2f,   0.0f, 0.0f, 1.0f,    1.0f, 1.0f,//37
        -1.5f, -10.8f, 0.2f,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
        -1.3f, -11.0f, 0.2f,   0.0f, 0.0f, 1.0f,    0.0f, 0.0f,//39
        -1.3f, -10.8f, 0.2f,   0.0f, 0.0f, 1.0f,    0.0f, 1.0f,//40
        -1.5f, -11.0f, 2.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//41
        -1.5f, -10.8f, 2.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, //42
        -1.3f, -11.0f, 2.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//43
        -1.3f, -10.8f, 2.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//44

        //column 2
        -1.1f, -11.0f, 0.2f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//45
        -1.1f, -10.8f, 0.2f,   0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//46
        -0.9f, -11.0f, 0.2f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//47
        -0.9f, -10.8f, 0.2f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//48
        -1.1f, -11.0f, 2.0f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//49
        -1.1f, -10.8f, 2.0f,   0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//50
        -0.9f, -11.0f, 2.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//51
        -0.9f, -10.8f, 2.0f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//52

        //column 3
        -0.7f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//53
        -0.7f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f, //54
        -0.5f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//55
        -0.5f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//56
        -0.7f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//57
        -0.7f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f, //58
        -0.5f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//59
        -0.5f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//60

        //column 4
        -0.3f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//61
        -0.3f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //62
        -0.1f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//63
        -0.1f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//64
        -0.3f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//65
        -0.3f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //66
        -0.1f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//67
        -0.1f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//68

        //column 5
        0.1f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//69
        0.1f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //70
        0.3f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//71
        0.3f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//72
        0.1f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//73
        0.1f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //74
        0.3f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//75
        0.3f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//76

        //column 6
        0.5f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//77
        0.5f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //78
        0.7f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//79
        0.7f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//80
        0.5f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//81
        0.5f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //82
        0.7f, -11.0f, 2.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//83
        0.7f, -10.8f, 2.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//84

        //column 7
        0.9f, -11.0f, 0.2f, 0.0f, 0.0f, 1.0f,    1.0f, 1.0f,//85
        0.9f, -10.8f, 0.2f, 0.0f, 0.0f, 1.0f,    1.0f, 0.0f,//86
        1.1f, -11.0f, 0.2f, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f,//87
        1.1f, -10.8f, 0.2f, 0.0f, 0.0f, 1.0f,    0.0f, 1.0f,//88
        0.9f, -11.0f, 2.0f, 0.0f, 0.0f, 1.0f,    1.0f, 1.0f,//89
        0.9f, -10.8f, 2.0f, 0.0f, 0.0f, 1.0f,    1.0f, 0.0f,//90
        1.1f, -11.0f, 2.0f, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f,//91
        1.1f, -10.8f, 2.0f, 0.0f, 0.0f, 1.0f,    0.0f, 1.0f,//92


        //column 8
        1.3f, -11.0f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, //93
        1.3f, -10.8f, 0.2f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, //94
        1.5, -11.0f, 0.2f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//95
        1.5, -10.8f, 0.2f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//96
        1.3, -11.0f, 2.0f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//97
        1.3, -10.8f, 2.0f,   0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//98
        1.5, -11.0f, 2.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//99
        1.5, -10.8f, 2.0f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//100

        //column9
            -1.5f, -10.2f, 0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//101
            -1.5f, -10.0f, 0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//102
            -1.3f, -10.2f, 0.2f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//103
            -1.3f, -10.0f, 0.2f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//104
            -1.5f, -10.2f, 2.0f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//105
            -1.5f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//106
            -1.3f, -10.2f, 2.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//107
            -1.3f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//108

            //column10
            -1.1f, -10.2f, 0.2f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f,//109
            -1.1f, -10.0f, 0.2f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f,//110
            -0.9f, -10.2f, 0.2f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//111
            -0.9f, -10.0f, 0.2f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,//112
            -1.1f, -10.2f, 2.0f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f,//113
            -1.1f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f,//114
            -0.9f, -10.2f, 2.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//115
            -0.9f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,//116

            //column11
            -0.7f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,//117
            -0.7f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,//118
            -0.5f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//119
            -0.5f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,//120
            -0.7f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,//121
            -0.7f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,//122
            -0.5f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//123
            -0.5f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,//124

            //column12
            -0.3f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,//125
            -0.3f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,//126
            -0.1f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//127
            -0.1f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,//128
            -0.3f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,//129
            -0.3f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,//130
            -0.1f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//131
            -0.1f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,//132

            //column 13
            0.1f, -10.2f, 0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//133
            0.1f, -10.0f, 0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//134
            0.3f, -10.2f, 0.2f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//135
            0.3f, -10.0f, 0.2f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//136
            0.1f, -10.2f, 2.0f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//137
            0.1f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//138
            0.3f, -10.2f, 2.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//139
            0.3f, -10.0f, 2.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//140

            //column 14
            0.5f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//141
            0.5f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//142
            0.7f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//143
            0.7f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//144
            0.5f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//145
            0.5f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//146
            0.7f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//147
            0.7f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//148

            //column 15
            0.9f, -10.2f, 0.2f,   0.0f, 0.0f, 1.0f,    1.0f, 1.0f,//149
            0.9f, -10.0f, 0.2f,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f,//150
            1.1f, -10.2f, 0.2f,   0.0f, 0.0f, 1.0f,    0.0f, 0.0f,//151
            1.1f, -10.0f, 0.2f,   0.0f, 0.0f, 1.0f,    0.0f, 1.0f,//152
            0.9f, -10.2f, 2.0f,   0.0f, 0.0f, 1.0f,    1.0f, 1.0f,//153
            0.9f, -10.0f, 2.0f,   0.0f, 0.0f, 1.0f,    1.0f, 0.0f,//154
            1.1f, -10.2f, 2.0f,   0.0f, 0.0f, 1.0f,    0.0f, 0.0f,//155
            1.1f, -10.0f, 2.0f,   0.0f, 0.0f, 1.0f,    0.0f, 1.0f,//156


            //column 16
            1.3f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//157
            1.3f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//158
            1.5f, -10.2f, 0.2f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//159
            1.5f, -10.0f, 0.2f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//160
            1.3f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f,//161
            1.3f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f,//162
            1.5f, -10.2f, 2.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f,//163
            1.5f, -10.0f, 2.0f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,//164

    };

    GLfloat poolVerts[] = {
        //reflection pool
        2.25f, 5.25f, 0.0f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,//165
        -2.25f, 5.25f, 0.0f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f,// 166
        -2.25f, -7.25f, 0.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,// 167
        2.25f, -7.25f, 0.0f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f,// 168
        2.25f, 5.25f, 0.25f,   0.0f, 0.0f, -1.0f,  1.0f, 1.0f,// 169
        -2.25f, 5.25f, 0.25f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,// 170
        -2.25f, -7.25f, 0.25f, 0.0f, 0.0f, -1.0f,  0.0f, 0.0f, // 171
        2.25f, -7.25f, 0.25f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,// 172


        //reflection pool border
        2.5f, 5.5f, 0.0f,    0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//  173
        -2.5f, 5.5f, 0.0f,   0.0f, 0.0f, 1.0f,  1.0f, 0.0f,//  174
        -2.5f, -8.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,//  175
        2.5f, -8.0f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f,//  176
        2.5f, 5.5f, 0.25f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,//  177
        -2.5f, 5.5f, 0.25f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, // 178
        -2.5f, -8.0f, 0.25f, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, // 179
        2.5f, -8.0f, 0.25f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, // 180
    };

    // If data to share position data
    GLushort monumentIndices[] = {
        0, 1, 3,
        0, 1, 2,
        0, 1, 5,
        0, 4, 5,
        0, 4, 6,
        0, 2, 6,
        1, 5, 7,
        1, 3, 7,
        2, 3, 7,
        2, 6, 7,
        4, 6, 5,
        5, 6, 7,
        4, 8, 6,
        4, 8, 5,
        5, 8, 7,
        7, 8, 6,
    };

    GLushort planeIndices[] = {
        0, 1, 2,
        1, 2, 3
    };

    GLushort buildingIndices[] = {
    0, 1, 2,
0, 2, 3,
2, 3, 6,
3, 6, 7,
1, 2, 6,
1, 5, 6,
0, 1, 5,
0, 1, 4,
0, 3, 7,
0, 4, 7,
4, 5, 6,
4, 6, 7,
8, 9, 13,
8, 12, 13,
8, 11, 15,
8, 12, 15,
11, 10, 14,
11, 14, 15,
9, 10, 14,
9, 13, 14,
12, 14, 15,
12, 13, 14,
17, 18, 19,
16, 18, 17,
17, 19, 23,
17, 21, 23,
18, 22, 23,
18, 19, 23,
16, 18, 22,
16, 20, 22,
16, 17, 20,
17, 20, 21,
    };

    GLushort columnIndices[] =
    {
        0, 1, 5,
0, 4, 5,
1, 3, 7,
1, 5, 7,
3, 2, 6,
3, 6, 7,
0, 4, 6,
0, 2, 6,
8, 9, 13,
8, 12, 13,
9, 11, 15,
9, 13, 15,
11, 10, 14,
11, 14, 15,
8, 12, 14,
8, 10, 14,
16, 17, 21,
16, 20, 21,
17, 19, 23,
17, 21, 23,
19, 18, 22,
19, 22, 23,
16, 20, 22,
16, 18, 22,
24, 25, 29,
24, 28, 29,
25, 27, 31,
25, 29, 31,
27, 26, 30,
27, 30, 31,
24, 28, 30,
24, 26, 30,
32, 33, 37,
32, 36, 37,
33, 35, 39,
33, 37, 39,
35, 34, 38,
35, 38, 39,
32, 36, 38,
32, 34, 38,
40, 41, 45,
40, 44, 45,
41, 43, 47,
41, 45, 47,
43, 42, 46,
43, 46, 47,
40, 44, 46,
40, 42, 46,
48, 49, 53,
48, 52, 53,
49, 51, 55,
49, 53, 55,
51, 50, 54,
51, 54, 55,
48, 52, 54,
48, 50, 54,
56, 57, 61,
56, 60, 61,
57, 59, 63,
57, 61, 63,
59, 58, 62,
59, 62, 63,
56, 60, 62,
56, 58, 62,
64, 65, 69,
64, 68, 69,
65, 67, 71,
65, 69, 71,
67, 66, 70,
67, 70, 71,
64, 68, 70,
64, 66, 70,
72, 73, 77,
72, 76, 77,
73, 75, 79,
73, 77, 79,
75, 74, 78,
75, 78, 79,
72, 76, 78,
72, 74, 78,
80, 81, 85,
80, 84, 85,
81, 83, 87,
81, 85, 87,
83, 82, 86,
83, 86, 87,
80, 84, 86,
80, 82, 86,
88, 89, 93,
88, 92, 93,
89, 91, 95,
89, 93, 95,
91, 90, 94,
91, 94, 95,
88, 92, 94,
88, 90, 94,
96, 97, 101,
96, 100, 101,
97, 99, 103,
97, 101, 103,
99, 98, 102,
99, 102, 103,
96, 100, 102,
96, 98, 102,
104, 105, 109,
104, 108, 109,
105, 107, 111,
105, 109, 111,
107, 106, 110,
107, 110, 111,
104, 108, 110,
104, 106, 110,
112, 113, 117,
112, 116, 117,
113, 115, 119,
113, 117, 119,
115, 114, 118,
115, 118, 119,
112, 116, 118,
112, 114, 118,
120, 121, 125,
120, 124, 125,
121, 123, 127,
121, 125, 127,
123, 122, 126,
123, 126, 127,
120, 124, 126,
120, 122, 126,
128, 129, 133,
128, 132, 133,
129, 131, 135,
129, 133, 135,
131, 130, 134,
131, 134, 135,
128, 132, 134,
128, 130, 134,
    };

    GLushort poolIndices[] = {

        //reflection pool and border
        1, 0, 3,
        1, 2, 3,
        1, 0, 4,
        1, 5, 4,
        1, 2, 6,
        1, 5, 6,
        2, 6, 7,
        2, 3, 7,
        0, 3, 7,
        0, 4, 7,
        5, 4, 6,
        4, 6, 7,
        5, 13, 12,
        5, 4, 12,
        4, 12, 15,
        7, 4, 15,
        6, 14, 15,
        6, 7, 15,
        14, 6, 13,
        13, 5, 6,
        9, 10, 14,
        9, 13, 14,
        10, 11, 14,
        11, 14, 15,
        8, 11, 15,
        8, 12, 15,
        8, 9, 13,
        8, 13, 12,
    };

    // Creates the Vertex Attribute Pointer for the screen coordinates
    const GLuint floatsPerVertex = 3; // Number of coordinates per vertex
    const GLuint floatsPerNormal = 3;  // (r, g, b, a)
    const GLuint floatsPerUV = 2;

    // Strides between vertex coordinates is 6 (x, y, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    glGenVertexArrays(5, mesh.VAO);  //generates 1 vertex array
    glGenBuffers(10, mesh.VBO);	     // generates two VBOs

    glBindVertexArray(mesh.VAO[0]);  //binds our VAO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[0]);	//binds our first VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(monumentVerts), monumentVerts, GL_STATIC_DRAW);	//sends the vertices to the buffer

    mesh.nMonumentIndices = sizeof(monumentIndices) / sizeof(monumentIndices[0]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[1]);  //binds second VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(monumentIndices), monumentIndices, GL_STATIC_DRAW);  //sends the indices to the buffer

    // Creates the Vertex Attribute Pointer
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glBindVertexArray(mesh.VAO[1]);  //binds our VAO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[2]);	//binds our first VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW);	//sends the vertices to the buffer

    mesh.nPlaneIndices = sizeof(planeIndices) / sizeof(planeIndices[0]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[3]);  //binds second VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);  //sends the indices to the buffer

    // Creates the Vertex Attribute Pointer
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glBindVertexArray(mesh.VAO[2]);  //binds our VAO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[4]);	//binds our first VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(buildingVerts), buildingVerts, GL_STATIC_DRAW);	//sends the vertices to the buffer

    mesh.nBuildingIndices = sizeof(buildingIndices) / sizeof(buildingIndices[0]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[5]);  //binds second VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buildingIndices), buildingIndices, GL_STATIC_DRAW);  //sends the indices to the buffer

    // Creates the Vertex Attribute Pointer
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glBindVertexArray(mesh.VAO[3]);  //binds our VAO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[6]);	//binds our first VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(columnVerts), columnVerts, GL_STATIC_DRAW);	//sends the vertices to the buffer

    mesh.nColumnIndices = sizeof(columnIndices) / sizeof(columnIndices[0]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[7]);  //binds second VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(columnIndices), columnIndices, GL_STATIC_DRAW);  //sends the indices to the buffer

    // Creates the Vertex Attribute Pointer
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glBindVertexArray(mesh.VAO[4]);  //binds our VAO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[8]);	//binds our first VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(poolVerts), poolVerts, GL_STATIC_DRAW);	//sends the vertices to the buffer

    mesh.nPoolIndices = sizeof(poolIndices) / sizeof(poolIndices[0]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[9]);  //binds second VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(poolIndices), poolIndices, GL_STATIC_DRAW);  //sends the indices to the buffer

    // Creates the Vertex Attribute Pointer
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    cout << "Mesh created" << endl;
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(5, mesh.VAO);
    glDeleteBuffers(10, mesh.VBO);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Set the texture wrapping parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        cout << "Tex parameters set" << endl;
        // Set texture filtering parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        cout << "Filtering parameters set" << endl;

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "Mipmaps generated" << endl;

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture.

        return true;
    }

    // Error loading the image
    return false;
}

