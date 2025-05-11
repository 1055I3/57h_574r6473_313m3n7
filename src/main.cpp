#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#define CHECK(retval, msg) do { \
        if(!retval) {           \
            std::cerr << "Urk! " << msg << std::endl; \
            glfwTerminate();    \
            return -1;          \
        }                       \
    } while(0)

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

/* settings */
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

/* camera */
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

/* timing */
float deltaTime = 0.0f;
float lastFrame = 0.0f;

/* spotlight */
bool spotSwitch = false;
float cutOff = glm::cos(glm::radians(10.0));
float outerCutOff = glm::cos(glm::radians(12.5));

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    PointLight pointLight;
    ProgramState() : camera(glm::vec3(0.0f, 0.0f, 5.7f)) {}
};

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "5th-stargate-element", NULL, NULL);
    if (window == NULL) {
        std::cout << "Urk! Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    /* ambient specular light color and other light values */
    float constant = 1.0,
            linear = 0.022,
            quadratic = 0.0019;
    glm::vec3 ambientColor = glm::vec3(0.158116f, 0.168f, 0.168f);
    glm::vec3 specularColor = glm::vec3(0.941176f, 1.0f, 1.0f);

    /* sun model vertices, matrices, textures, shaders */
    Model sunModel("resources/objects/sun_v3/sun_model.obj");
    Shader sunShader("resources/shaders/2_vertex_shader.vs", "resources/shaders/2_fragment_shader.fs");
    glm::mat4 sunModelMatrix;
    glm::vec3 sunPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 sunColor = glm::vec3(1.0f, 1.0f, 0.22f);

    /* mercury model vertices, matrices, textures, shaders */
    Model mercuryModel("resources/objects/mercury_v1/mercury_model.obj");
    Shader mercuryShader("resources/shaders/3_vertex_shader.vs", "resources/shaders/3_fragment_shader.fs");
    mercuryModel.SetShaderTextureNamePrefix("material.");
    glm::mat4 mercuryModelMatrix, mercuryNormalMatrix;
    mercuryShader.use();
    mercuryShader.setFloat("material.shininess", 128.0f);
    mercuryShader.setVec3("pointLight.position", sunPosition);
    mercuryShader.setVec3("pointLight.ambient", ambientColor);
    mercuryShader.setVec3("pointLight.diffuse", sunColor);
    mercuryShader.setVec3("pointLight.specular", specularColor);
    mercuryShader.setFloat("pointLight.constant", constant);
    mercuryShader.setFloat("pointLight.linear", linear);
    mercuryShader.setFloat("pointLight.quadratic", quadratic);
    mercuryShader.setFloat("spotLight.cutOff", cutOff);
    mercuryShader.setFloat("spotLight.outerCutOff", outerCutOff);
    mercuryShader.setFloat("spotLight.constant", constant);
    mercuryShader.setFloat("spotLight.linear", linear);
    mercuryShader.setFloat("spotLight.quadratic", quadratic);
    mercuryShader.setVec3("spotLight.diffuse", specularColor);
    mercuryShader.setVec3("spotLight.specular", specularColor);

    /* tetrahedron vertices, matrices, textures, shaders */
    float tetrahedron[] = {
         /* x    y           z               normals                                 texture */
            -1,  -0.816496,  -0.866025,      0,          -1,          0,             -0.5,   0,
            1,   -0.816496,  -0.866025,      0,          -1,          0,             0.5,    0,
            0,   -0.816496,  0.866025,       0,          -1,          0,             0,      1,

            -1,  -0.816496,  -0.866025,      0,          0.468521,    -0.883452,     -0.5,   0,
            1,   -0.816496,  -0.866025,      0,          0.468521,    -0.883452,     0.5,    0,
            0,   0.816496,   0,              0,          0.468521,    -0.883452,     0,      -1,

            1,   -0.816496,  -0.866025,      0.837096,   0.256307,    0.483298,      0.5,    0,
            0,   -0.816496,  0.866025,       0.837096,   0.256307,    0.483298,      0,      1,
            0,   0.816496,   0,              0.837096,   0.256307,    0.483298,      1,      1,

            -1,  -0.816496,  -0.866025,      -0.837096,  0.256307,    0.483298,      -0.5,   0,
            0,   -0.816496,  0.866025,       -0.837096,  0.256307,    0.483298,      0,      1,
            0,   0.816496,   0,              -0.837096,  0.256307,    0.483298,      -1,     1
    };
    unsigned tetraIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    unsigned tetraEBO, tetraVBO, tetraVAO;
    glGenVertexArrays(1, &tetraVAO);
    glGenBuffers(1, &tetraVBO);
    glGenBuffers(1, &tetraEBO);
    glBindVertexArray(tetraVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tetraVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tetrahedron), tetrahedron, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tetraEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tetraIndices), tetraIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *) (3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *) (6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    glm::mat4 tetraModelMatrix1 = glm::mat4(1.0);
    tetraModelMatrix1 = glm::translate(tetraModelMatrix1, glm::vec3(-9.0f, 0.0f, -7.794229f));
    tetraModelMatrix1 = glm::scale(tetraModelMatrix1, glm::vec3(1.4f, 1.4f, 1.4f));
    glm::mat4 tetraModelMatrix2 = glm::mat4(1.0);
    tetraModelMatrix2 = glm::translate(tetraModelMatrix2, glm::vec3(9.0f, 0.0f, -7.794229f));
    tetraModelMatrix2 = glm::scale(tetraModelMatrix2, glm::vec3(1.4f, 1.4f, 1.4f));
    glm::mat4 tetraModelMatrix3 = glm::mat4(1.0);
    tetraModelMatrix3 = glm::translate(tetraModelMatrix3, glm::vec3(0.0f, 0.0f, 7.794229f));
    tetraModelMatrix3 = glm::scale(tetraModelMatrix3, glm::vec3(1.4f, 1.4f, 1.4f));

    Shader tetraShader("resources/shaders/1_vertex_shader.vs", "resources/shaders/1_fragment_shader.fs");

    unsigned tetraTex[2];
    unsigned char *data;
    int width, height, nChannels;
    glGenTextures(2, tetraTex);
    glBindTexture(GL_TEXTURE_2D, tetraTex[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    data = stbi_load("resources/textures/Marble009_1K_Color.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Specular map marble failed to load! Terminating...");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    data = stbi_load("resources/textures/Marble009_1K_Displacement.png", &width, &height, &nChannels, 0);
    glBindTexture(GL_TEXTURE_2D, tetraTex[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK(data, "Fatal error! Specular map marble failed to load! Terminating...");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    tetraShader.use();
    tetraShader.setInt("materDiffuse", 0);
    tetraShader.setInt("materSpecular", 1);
    tetraShader.setFloat("materShininess", 38.4f);
    tetraShader.setVec3("lightDiffuse", sunColor);
    tetraShader.setVec3("lightAmbient", ambientColor);
    tetraShader.setVec3("lightSpecular", specularColor);
    tetraShader.setVec3("lightPosition", sunPosition);
    tetraShader.setFloat("lightConstant", constant);
    tetraShader.setFloat("lightLinear", linear);
    tetraShader.setFloat("lightQuadratic", quadratic);
    tetraShader.setFloat("spotCutOff", cutOff);
    tetraShader.setFloat("spotOuterCutOff", outerCutOff);

    /* skybox nebula */
    float nebula[] = {
          /* x    y      z */
            -1.0, 1.0,  -1.0,
            -1.0, -1.0, -1.0,
            1.0,  -1.0, -1.0,
            1.0,  -1.0, -1.0,
            1.0,  1.0,  -1.0,
            -1.0, 1.0,  -1.0,

            -1.0, -1.0, 1.0,
            -1.0, -1.0, -1.0,
            -1.0, 1.0,  -1.0,
            -1.0, 1.0,  -1.0,
            -1.0, 1.0,  1.0,
            -1.0, -1.0, 1.0,

            1.0,  -1.0, -1.0,
            1.0,  -1.0, 1.0,
            1.0,  1.0,  1.0,
            1.0,  1.0,  1.0,
            1.0,  1.0,  -1.0,
            1.0,  -1.0, -1.0,

            -1.0, -1.0, 1.0,
            -1.0, 1.0,  1.0,
            1.0,  1.0,  1.0,
            1.0,  1.0,  1.0,
            1.0,  -1.0, 1.0,
            -1.0, -1.0, 1.0,

            -1.0, 1.0,  -1.0,
            1.0,  1.0,  -1.0,
            1.0,  1.0,  1.0,
            1.0,  1.0,  1.0,
            -1.0, 1.0,  1.0,
            -1.0, 1.0,  -1.0,

            -1.0, -1.0, -1.0,
            -1.0, -1.0, 1.0,
            1.0,  -1.0, -1.0,
            1.0,  -1.0, -1.0,
            -1.0, -1.0, 1.0,
            1.0,  -1.0, 1.0
    };

    unsigned nebulaVAO, nebulaVBO;
    glGenVertexArrays(1, &nebulaVAO);
    glGenBuffers(1, &nebulaVBO);
    glBindVertexArray(nebulaVAO);
    glBindBuffer(GL_ARRAY_BUFFER, nebulaVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(nebula), &nebula, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float),  (void *) 0);
    glBindVertexArray(0);

    Shader nebulaShader("resources/shaders/4_vertex_shader.vs", "resources/shaders/4_fragment_shader.fs");

    unsigned nebulaTex;
    glGenTextures(1, &nebulaTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, nebulaTex);
    data = stbi_load("resources/textures/skybox_right.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Failed to load skybox_right! Terminating...");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    data = stbi_load("resources/textures/skybox_left.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Failed to load skybox_left! Terminating...");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    data = stbi_load("resources/textures/skybox_bottom.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Failed to load skybox_top! Terminating...");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    data = stbi_load("resources/textures/skybox_top.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Failed to load skybox_bottom! Terminating...");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    data = stbi_load("resources/textures/skybox_front.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Failed to load skybox_back! Terminating...");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    data = stbi_load("resources/textures/skybox_back.png", &width, &height, &nChannels, 0);
    CHECK(data, "Fatal error! Failed to load skybox_front! Terminating...");
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    nebulaShader.use();
    nebulaShader.setInt("skyBox", 0);

    /* blur and bloom framebuffer */
    float quad[] = {
          /* x     y      texture */
            -1.0,  1.0,   0.0,  1.0,
            -1.0,  -1.0,  0.0,  0.0,
            1.0,   -1.0,  1.0,  0.0,

            -1.0,  1.0,   0.0,  1.0,
            1.0,   -1.0,  1.0,  0.0,
            1.0,   1.0,   1.0,  1.0
    };

    unsigned bloomVAO, bloomVBO;
    glGenVertexArrays(1, &bloomVAO);
    glGenBuffers(1, &bloomVBO);
    glBindVertexArray(bloomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bloomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),  (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),  (void *) (2*sizeof(float)));
    glBindVertexArray(0);

    unsigned bloomFBO;
    glGenFramebuffers(1, &bloomFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);

    unsigned bloomColorBuffer[2];
    glGenTextures(2, bloomColorBuffer);
    glBindTexture(GL_TEXTURE_2D, bloomColorBuffer[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomColorBuffer[0], 0);

    glBindTexture(GL_TEXTURE_2D, bloomColorBuffer[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bloomColorBuffer[1], 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    unsigned attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    unsigned bloomRenderBuffer;
    glGenRenderbuffers(1, &bloomRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, bloomRenderBuffer);
    glad_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, bloomRenderBuffer);

    CHECK((glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE), "Fatal error! Framebuffer incomplete! Terminating...");

    unsigned blurFBO[2];
    glGenFramebuffers(2, blurFBO);
    unsigned blurColorBuffer[2];
    glGenTextures(2, blurColorBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
    glBindTexture(GL_TEXTURE_2D, blurColorBuffer[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurColorBuffer[0], 0);
    CHECK((glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE), "Fatal error! Framebuffer incomplete! Terminating...");

    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[1]);
    glBindTexture(GL_TEXTURE_2D, blurColorBuffer[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurColorBuffer[1], 0);
    CHECK((glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE), "Fatal error! Framebuffer incomplete! Terminating...");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Shader blurShader("resources/shaders/5_vertex_shader.vs", "resources/shaders/5_fragment_shader.fs");
    blurShader.use();
    blurShader.setInt("image", 0);

    Shader outputShader("resources/shaders/6_vertex_shader.vs", "resources/shaders/6_fragment_shader.fs");
    outputShader.use();
    outputShader.setInt("baseImage", 0);
    outputShader.setInt("highlights", 1);

    // draw in wireframe
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    /* loop variables */
    float currentFrame, t;
    unsigned blurSwitch;
    glm::mat4 projection, view;

    /* render loop */
    while (!glfwWindowShouldClose(window)) {
        /* per-frame time logic */
        currentFrame = (float) glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
         t = currentFrame / 3;

        /* view projection transformations */
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();

        /* bloom framebuffer setup */
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        /* tetrahedron render */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tetraTex[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tetraTex[1]);
        glBindVertexArray(tetraVAO);
        tetraShader.use();
        tetraShader.setVec3("viewPosition", programState->camera.Position);
        tetraShader.setVec3("viewDirection", programState->camera.Front);
        tetraShader.setBool("spotToggle", spotSwitch);
        tetraShader.setMat4("view", view);
        tetraShader.setMat4("projection", projection);
        tetraShader.setMat4("model", tetraModelMatrix1);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        tetraShader.setMat4("model", tetraModelMatrix2);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
        tetraShader.setMat4("model", tetraModelMatrix3);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        glBindVertexArray(0);

        /* sun render */
        sunShader.use();
        sunShader.setMat4("projection", projection);
        sunShader.setMat4("view", view);
        sunModelMatrix = glm::mat4(1.0f);
        sunModelMatrix = glm::rotate(sunModelMatrix, -currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        sunShader.setMat4("model", sunModelMatrix);
        sunModel.Draw(sunShader);

        /* mercury render */
        mercuryShader.use();
        mercuryShader.setVec3("spotLight.position", programState->camera.Position);
        mercuryShader.setVec3("spotLight.direction", programState->camera.Front);
        mercuryShader.setBool("spotLight.spotToggle", spotSwitch);
        mercuryShader.setVec3("viewPosition", programState->camera.Position);
        mercuryShader.setMat4("projection", projection);
        mercuryShader.setMat4("view", view);
        mercuryModelMatrix = glm::mat4(1.0f);
        mercuryModelMatrix = glm::translate(mercuryModelMatrix, glm::vec3((float) 5*cos(t), 0.0f, (float) 5*sin(t)));
        mercuryModelMatrix = glm::rotate(mercuryModelMatrix, currentFrame, glm::vec3(0.0, 1.0, 0.0));
        mercuryNormalMatrix = glm::mat4(1.0f);
        mercuryNormalMatrix =  glm::rotate(mercuryNormalMatrix, currentFrame, glm::vec3(0.0, 1.0, 0.0));
        mercuryShader.setMat4("model", mercuryModelMatrix);
        mercuryShader.setMat4("normRotation", mercuryNormalMatrix);
        mercuryModel.Draw(mercuryShader);

        /* nebula render */
        glDepthFunc(GL_LEQUAL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, nebulaTex);
        glBindVertexArray(nebulaVAO);
        nebulaShader.use();
        nebulaShader.setMat4("projection", projection);
        nebulaShader.setMat4("view", glm::mat4(glm::mat3(view)));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);
        glBindVertexArray(0);

        /* blur the highlights */
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloomColorBuffer[1]);
        glBindVertexArray(bloomVAO);
        blurShader.use();
        blurShader.setBool("blurToggle", true);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        blurSwitch = false;
        for (unsigned i = 0; i < 15; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[blurSwitch]);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, blurColorBuffer[!blurSwitch]);
            blurShader.setBool("blurToggle", blurSwitch);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            blurSwitch = !blurSwitch;
        }
        glBindVertexArray(0);

        /* screen output */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloomColorBuffer[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blurColorBuffer[!blurSwitch]);
        glBindVertexArray(bloomVAO);
        outputShader.use();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        /* imgui thing */
        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        /* swap buffers and poll events */
        glfwSwapBuffers(window);
        glfwPollEvents();
        processInput(window);
    }

    /* free memory and terminate */
    glDeleteVertexArrays(1, &tetraVAO);
    glDeleteBuffers(1, &tetraVBO);
    glDeleteTextures(2, tetraTex);
    glDeleteVertexArrays(1, &nebulaVAO);
    glDeleteBuffers(1, &nebulaVBO);
    glDeleteTextures(1, &nebulaTex);
    glDeleteVertexArrays(1, &bloomVAO);
    glDeleteBuffers(1, &bloomVBO);
    glDeleteTextures(2, bloomColorBuffer);
    glDeleteRenderbuffers(1, &bloomRenderBuffer);
    glDeleteFramebuffers(1, &bloomFBO);
    glDeleteTextures(2, blurColorBuffer);
    glDeleteFramebuffers(2, blurFBO);
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        spotSwitch = true;
    if (key == GLFW_KEY_F && action == GLFW_RELEASE)
        spotSwitch = false;
}
