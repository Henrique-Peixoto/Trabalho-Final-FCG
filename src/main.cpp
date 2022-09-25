#include <bits/stdc++.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <array>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
//#include "collisions.h"

// IDs dos objetos
#define PLATFORM 0
#define PLAYER 1
#define KEY 2

struct ObjModel {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true) {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

GLuint BuildPlatform();
void BuildTrianglesAndAddToVirtualScene(ObjModel*);
void ComputeNormals(ObjModel* model);
void LoadShadersFromFiles();
void DrawVirtualObject(const char* object_name);
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);
void LoadAllObjFiles();
glm::mat4 PlatformBaseModel();
glm::mat4 PlayerBaseModel();
void ApplyPlataformTransformation();
void Draw(const char* objectName, glm::mat4 model, int objectNumber);
void DrawLevel2();
void MovePlayer(glm::vec4 camera_view_vector, glm::vec4 camera_up_vector, float delta_t);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Funções relacionadas às texturas
void LoadTextureImage(const char* filename);
void LoadAllTextures();

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject {
    std::string name;
    size_t first_index;
    size_t num_indices;
    GLenum rendering_mode;
    GLuint vertex_array_object_id;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
    glm::vec3 bottom_left_back;
    glm::vec3 up_right_front;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Variáveis usadas para controlar o movimento da câmera livre
float g_CameraTheta = 0.8f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.6f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 2.5f; // Distância da câmera para a origem

// Variáveis que controlam a movimentação da câmera
bool g_WPressed = false; // Andar para frente
bool g_SPressed = false; // Andar para trás
bool g_APressed = false; // Andar para esquerda
bool g_DPressed = false; // Andar para direita
bool g_SpacebarPressed = false; // Pular

// Variável que controla a velocidade de movimento da câmera
float cameraSpeed = 1.0f;

// Variável que controla o estado do botão esquerdo do mouse
bool g_LeftMouseButtonPressed = false;

// Ângulos de Euler
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// Controle dos valores de x e y do mouse
double g_LastCursorPosX;
double g_LastCursorPosY;

// Posição atual do jogador e possível posição futura
glm::vec4 g_PlayerPosition;
glm::vec4 g_NewPlayerPosition;

// Valores de tempo
float g_CurrentTime = 0;
float g_LastTimeJumped = 0;

// Colisões
std::map<std::string, bool> CheckCollision();
void CalculateHitBox(std::string objName, float x, float y, float z, float scaleFactorX = 1.0, float scaleFactorY = 1.0, float scaleFactorZ = 1.0);
std::vector<SceneObject> g_HitBoxes;

// Funções e variáveis para curvas de Bézier
void initializeBezierCurves();
void buildBezierCurveXTranslation();
void buildBezierCurveYTranslation();
void buildBezierCurveDegree2();
void buildBezierCurveDegree3();
std::map<std::string, float> retrieveBezierCurvePoint(std::string curveName);
std::map<std::string, std::map<std::string, std::array<float, 200>>> g_BezierCurves;

int main(int argc, char* argv[]) {
    int success = glfwInit();
    if (!success) {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "O nome do jogo vai aqui", NULL, NULL);
    if (!window) {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    glfwSetWindowSize(window, 800, 800);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    //FramebufferSizeCallback(window, 800, 600);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    LoadShadersFromFiles();

    // Construindo uma plataforma
    GLuint vertex_array_object_id = BuildPlatform();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float previous_time = (float)glfwGetTime();
    float current_time = (float)glfwGetTime();
    float delta_t = current_time - previous_time;
    previous_time = current_time;

    //GLint model_uniform = glGetUniformLocation(program_id, "model");
    GLint view_uniform = glGetUniformLocation(program_id, "view");
    GLint projection_uniform = glGetUniformLocation(program_id, "projection");
    //GLint render_as_black_uniform = glGetUniformLocation(program_id, "render_as_black");

    if (argc > 1) {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    LoadAllTextures();
    LoadAllObjFiles();

    g_PlayerPosition = glm::vec4(0.0f, 3.0f, 0.0f, 1.0f);
    g_NewPlayerPosition = glm::vec4(0.0f, 3.0f, 0.0f, 1.0f);

    initializeBezierCurves();

    while (!glfwWindowShouldClose(window)) {
        // Inicialização
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program_id);

        glBindVertexArray(vertex_array_object_id);

        float r = g_CameraDistance;
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);

        glm::vec4 camera_position_c = glm::vec4(x+g_PlayerPosition.x, y+g_PlayerPosition.y, z+g_PlayerPosition.z, 1.0f);
        glm::vec4 camera_lookat_l = g_PlayerPosition;
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c;
        glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));

        // vvvvvvvv Cálculos relacionados à matriz "projection" vvvvvvvv
        glm::mat4 projection;
        // O bloco deve se mover como uma câmera livre ao pressionar as teclas
        // WASD. Quando o mouse for movido, deve ser aplicada uma câmera look-at com o bloco (personagem) centralizado.

        glm::mat4 model = Matrix_Identity()
                * Matrix_Translate(
                    g_PlayerPosition.x,
                    g_PlayerPosition.y,
                    g_PlayerPosition.z
                )
                * Matrix_Rotate_Y(g_CameraTheta) * Matrix_Rotate_Y(3.1415);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLAYER);
        DrawVirtualObject("player");

        float nearplane = -0.1f;
        float farplane = -50.0f;

        float fieldOfView = 3.141592 / 3.0f;
        projection = Matrix_Perspective(fieldOfView, g_ScreenRatio, nearplane, farplane);
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
        // ^^^^^^^^^ Cálculos relacionados à matriz "projection" ^^^^^^^^^

        
        DrawLevel2();
        MovePlayer(camera_view_vector, camera_up_vector, delta_t);
        std::map<std::string, bool> collisions = CheckCollision();
        if (!(collisions["x"] && collisions["y"] && collisions["z"])) {
            g_NewPlayerPosition = glm::vec4(g_NewPlayerPosition.x, g_NewPlayerPosition.y-(delta_t*cameraSpeed*3), g_NewPlayerPosition.z, g_NewPlayerPosition.w);
        }
        g_HitBoxes.clear();
        g_PlayerPosition = g_NewPlayerPosition;

        glBindVertexArray(0);

        current_time = glfwGetTime();
        delta_t = current_time - previous_time;
        previous_time = current_time;
        g_CurrentTime = glfwGetTime();


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

std::map<std::string, bool> CheckCollision() {
    bool collisionX = false;
    bool collisionY = false;
    bool collisionZ = false;
    SceneObject &player = g_VirtualScene["player"];

    for (SceneObject platform : g_HitBoxes) {
        player.bottom_left_back = glm::vec3(
            g_NewPlayerPosition.x+player.bbox_min.x,
            g_NewPlayerPosition.y+player.bbox_min.y,
            g_NewPlayerPosition.z+player.bbox_min.z
        );
        player.up_right_front = glm::vec3(
            g_NewPlayerPosition.x+player.bbox_max.x,
            g_NewPlayerPosition.y+player.bbox_max.y,
            g_NewPlayerPosition.z+player.bbox_max.z
        );

        collisionX = player.up_right_front.x >= platform.bottom_left_back.x
                   && platform.up_right_front.x >= player.bottom_left_back.x;
        collisionY = player.up_right_front.y >= platform.bottom_left_back.y
                   && platform.up_right_front.y >= player.bottom_left_back.y;
        collisionZ = player.up_right_front.z >= platform.bottom_left_back.z
                   && platform.up_right_front.z >= player.bottom_left_back.z;

       if (collisionX && collisionY && collisionZ) break;
    }

    std::map<std::string, bool> collisions;
    collisions["x"] = collisionX;
    collisions["y"] = collisionY;
    collisions["z"] = collisionZ;

    return collisions;
}

void MovePlayer(glm::vec4 camera_view_vector, glm::vec4 camera_up_vector, float delta_t) {
        glm::vec4 camera_view_vector_n = normalize(camera_view_vector);
        camera_view_vector_n = glm::vec4(camera_view_vector_n.x*3, camera_view_vector_n.y*3, camera_view_vector_n.z*3, camera_view_vector_n.w);
        glm::vec4 wsMoveDirection = glm::vec4(camera_view_vector_n.x, 0.0f, camera_view_vector_n.z, 0.0f);
        if (g_WPressed) g_NewPlayerPosition = g_PlayerPosition + wsMoveDirection * cameraSpeed * delta_t;
        if (g_SPressed) g_NewPlayerPosition = g_PlayerPosition - wsMoveDirection * cameraSpeed * delta_t;

        glm::vec4 sideVector = crossproduct(camera_view_vector_n, camera_up_vector);
        glm::vec4 adMoveDirection = glm::vec4(sideVector.x, 0.0f, sideVector.z, 0.0f);
        if (g_APressed) g_NewPlayerPosition = g_PlayerPosition - adMoveDirection * cameraSpeed * delta_t;
        if (g_DPressed) g_NewPlayerPosition = g_PlayerPosition + adMoveDirection * cameraSpeed * delta_t;

        if (g_SpacebarPressed) g_NewPlayerPosition = glm::vec4(
            g_NewPlayerPosition.x, 
            g_NewPlayerPosition.y + 0.5f, 
            g_NewPlayerPosition.z, 
            g_NewPlayerPosition.w
        );
}

void LoadAllTextures() {
    LoadTextureImage("../../data/level2-platform-texture.jpg");
    LoadTextureImage("../../data/level2-player-texture.jpg");
    LoadTextureImage("../../data/level2-key-texture.jpg");
}

glm::mat4 PlatformBaseModel() {
    glm::mat4 model = Matrix_Identity()
                    * Matrix_Translate(0.0f, 0.0f, 0.0f);
                    //Matrix_Scale(0.5f, 0.5f, 0.5f);

    return model;
}

glm::mat4 PlayerBaseModel() {
    glm::mat4 model = Matrix_Identity()
                    * Matrix_Translate(0.0f, 1.5f, 0.0f);
                    //Matrix_Scale(0.25f, 0.5f, 0.25f);

    return model;
}

void CalculateHitBox(std::string objName, float x, float y, float z, float scaleFactorX /*= 1.0*/, float scaleFactorY /* = 1.0*/, float scaleFactorZ /*= 1.0*/) {
    SceneObject &obj = g_VirtualScene[objName];
    obj.bottom_left_back = glm::vec3(
        x+obj.bbox_min.x*scaleFactorX, 
        y+obj.bbox_min.y*scaleFactorY, 
        z+obj.bbox_min.z*scaleFactorZ
    );
    obj.up_right_front = glm::vec3(
        x+obj.bbox_max.x*scaleFactorX, 
        y+obj.bbox_max.y*scaleFactorY, 
        z+obj.bbox_max.z*scaleFactorZ
    );
}

bool trocou = false;
bool invertAnimationDirection = false;
void ApplyPlataformTransformation() {
    CalculateHitBox("platform", 0.0f, 2.0f, 0.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    glm::mat4 model = PlatformBaseModel() * Matrix_Translate(0.0f, 2.0f, 0.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 2.0f, 0.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 2.0f, 0.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 8.0f, 2.0f, 0.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 2.0f, 0.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 8.0f, 2.0f, 4.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 2.0f, 4.0f);
    Draw("platform", model, PLATFORM);

    std::map<std::string, float> point = retrieveBezierCurvePoint("x_translate");
    CalculateHitBox("platform", 8.0f+point["x"], 3.0f, 8.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f+point["x"], 3.0f, 8.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 8.0f, 3.0f, 12.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 3.0f, 12.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 8.0f, 3.0f, 14.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 3.0f, 14.0f) * Matrix_Rotate_Z(g_CurrentTime*2) * Matrix_Rotate_X(3.1415/2) * Matrix_Scale(0.5, 1, 2);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 8.0f, 3.0f, 16.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 3.0f, 16.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("y_translate");
    CalculateHitBox("platform", 8.0f, 3.0f+point["y"], 20.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 3.0f+point["y"], 20.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("parable");
    CalculateHitBox("platform", 8.0f+point["x"], 3.0f+point["y"], 24.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f+point["x"], 3.0f+point["y"], 24.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 8.0f, 3.0f, 28.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 3.0f, 28.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("cubic");
    CalculateHitBox("platform", 8.0f, 3.0f+point["y"], 32.0f+point["x"]);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f, 3.0f+point["y"], 32.0f+point["x"]);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 4.0f, 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 4.0f, 32.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("y_translate");
    CalculateHitBox("platform", 0.0f, 5.0f+point["y"], 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(0.0f, 5.0f+point["y"], 32.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("y_translate");
    CalculateHitBox("platform", -4.0f, 6.0f+point["y"], 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 6.0f+point["y"], 32.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("x_translate");
    CalculateHitBox("platform", -4.0f+point["x"], 8.0f, 28.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f+point["x"], 8.0f, 28.0f);
    Draw("platform", model, PLATFORM);

    point = retrieveBezierCurvePoint("x_translate");
    CalculateHitBox("platform", -4.0f, 8.0f+point["x"], 24.0f+point["x"]);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 8.0f+point["x"], 24.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 6.0f, 19.45f, 1.0, 1.0, 0.25);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 6.0f, 19.45f) * Matrix_Scale(1, 1, 0.25);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 6.5f, 18.95f, 1.0, 1.0, 0.25);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 6.5f, 18.95f) * Matrix_Scale(1, 1, 0.25);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 7.0f, 18.45f, 1.0, 1.0, 0.25);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 7.0f, 18.45f) * Matrix_Scale(1, 1, 0.25);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 7.5f, 15.0f, 3.2, 1.0, 3.2);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 7.5f, 15.0f) * Matrix_Scale(3.2, 1, 3.2);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 8.0f, 15.0f, 2.0, 1.0, 2.0);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 8.0f, 15.0f) * Matrix_Scale(2, 1, 2);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 8.5f, 15.0f, 1.5, 1.0, 1.5);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 8.5f, 15.0f) * Matrix_Scale(1.5, 1, 1.5);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 9.0f, 15.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 9.0f, 15.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 9.0f, 15.0f, 0.5, 2.0, 0.5);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 9.0f, 15.0f) * Matrix_Scale(0.5, 2, 0.5);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -1.5f, 9.75f, 17.5f, 0.5, 8.0, 0.5);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-1.5f, 9.75f, 17.5f) * Matrix_Scale(0.5, 8, 0.5);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -6.5f, 9.75f, 17.5f, 0.5, 8.0, 0.5);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-6.5f, 9.75f, 17.5f) * Matrix_Scale(0.5, 8, 0.5);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -1.5f, 9.75f, 12.5f, 0.5, 8.0, 0.5);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-1.5f, 9.75f, 12.5f) * Matrix_Scale(0.5, 8, 0.5);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -6.5f, 9.75f, 12.5f, 0.5, 8.0, 0.5);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-6.5f, 9.75f, 12.5f) * Matrix_Scale(0.5, 8, 0.5);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("key", -4.0f, 14.0f, 15.0f);
    g_HitBoxes.push_back(g_VirtualScene["key"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 9.75f, 15.0f) * Matrix_Rotate_Y(3.1415/2) * Matrix_Rotate_Z(3.1415/2) * Matrix_Scale(0.25, 0.25, 0.25);
    Draw("key", model, KEY);
}

void Draw(const char* objectName, glm::mat4 model, int objectNumber) {
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, objectNumber);
    DrawVirtualObject(objectName);
}

void DrawLevel2() {
    ApplyPlataformTransformation();
}

std::map<std::string, float> retrieveBezierCurvePoint(std::string curveName) {
    float auxTime = g_CurrentTime;
    int remainder200 = (int)(g_CurrentTime*100)%200;

    if (remainder200 >= 194 && !trocou) {
        invertAnimationDirection = !invertAnimationDirection;
        trocou = true;
    }

    if (remainder200 >= 194 && remainder200 <= 199) {
        if (invertAnimationDirection) {
            auxTime = 0;
        } else {
            auxTime = 0.01;
        }
    }

    if (remainder200 < 194) trocou = false;

    std::array<float, 200> curvePointsX = g_BezierCurves[curveName]["x"];
    std::array<float, 200> curvePointsY = g_BezierCurves[curveName]["y"];

    float currentX, currentY;
    if (invertAnimationDirection) {
        currentX = curvePointsX[199-(int)(auxTime*100)%200];
        currentY = curvePointsY[199-(int)(auxTime*100)%200];
    } else {
        currentX = curvePointsX[(int)(auxTime*100)%200];
        currentY = curvePointsY[(int)(auxTime*100)%200];
    }

    std::map<std::string, float> point;
    point["x"] = currentX;
    point["y"] = currentY;

    return point;
}

void initializeBezierCurves() {
    buildBezierCurveXTranslation();
    buildBezierCurveYTranslation();
    buildBezierCurveDegree2();
    buildBezierCurveDegree3();
}

void buildBezierCurveXTranslation() {
    float controlPointsX[2];
    controlPointsX[0] = -2.0;
    controlPointsX[1] = 2.0;

    float controlPointsY[2];
    controlPointsY[0] = 0.0;
    controlPointsY[1] = 0.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    for (i = 0, t = 0; t <= 1; i++, t += 0.005) {
        curvePointsX[i] = controlPointsX[0] + t*(controlPointsX[1] - controlPointsX[0]);

        curvePointsY[i] = controlPointsY[0] + t*(controlPointsY[1] - controlPointsY[0]);
    }

    g_BezierCurves["x_translate"]["x"] = curvePointsX;
    g_BezierCurves["x_translate"]["y"] = curvePointsY;
}

void buildBezierCurveYTranslation() {
    float controlPointsX[2];
    controlPointsX[0] = 0.0;
    controlPointsX[1] = 0.0;

    float controlPointsY[2];
    controlPointsY[0] = -2.0;
    controlPointsY[1] = 2.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    for (i = 0, t = 0; t <= 1; i++, t += 0.005) {
        curvePointsX[i] = controlPointsX[0] + t*(controlPointsX[1] - controlPointsX[0]);

        curvePointsY[i] = controlPointsY[0] + t*(controlPointsY[1] - controlPointsY[0]);
    }

    g_BezierCurves["y_translate"]["x"] = curvePointsX;
    g_BezierCurves["y_translate"]["y"] = curvePointsY;
}

void buildBezierCurveDegree2() {
    float controlPointsX[3];
    controlPointsX[0] = -1.0;
    controlPointsX[1] = 0.0;
    controlPointsX[2] = 1.0;

    float controlPointsY[3];
    controlPointsY[0] = -1.0;
    controlPointsY[1] = 1.0;
    controlPointsY[2] = -1.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    for (i = 0, t = 0; t <= 1; i++, t += 0.005) {
        curvePointsX[i] = (1-t)*(1-t)*controlPointsX[0]
                        + 2*t*(1-t)*controlPointsX[1]
                        + t*t*controlPointsX[2];

        curvePointsY[i] = (1-t)*(1-t)*controlPointsY[0]
                        + 2*t*(1-t)*controlPointsY[1]
                        + t*t*controlPointsY[2];
    }

    g_BezierCurves["parable"]["x"] = curvePointsX;
    g_BezierCurves["parable"]["y"] = curvePointsY;
}

void buildBezierCurveDegree3() {
    float controlPointsX[3];
    controlPointsX[0] = 0.0;
    controlPointsX[1] = -3.0;
    controlPointsX[2] = 3.0;
    controlPointsX[3] = 0.0;

    float controlPointsY[3];
    controlPointsY[0] = 0.0;
    controlPointsY[1] = 3.0;
    controlPointsY[2] = 3.0;
    controlPointsY[3] = 0.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    for (i = 0, t = 0; t <= 1; i++, t += 0.005) {
        curvePointsX[i] = (1-t)*(1-t)*(1-t)*controlPointsX[0]
                        + 3*t*t*(1-t)*controlPointsX[1]
                        + 3*t*(1-t)*(1-t)*controlPointsX[2]
                        + t*t*t*controlPointsX[3];

        curvePointsY[i] = (1-t)*(1-t)*(1-t)*controlPointsY[0]
                        + 3*t*t*(1-t)*controlPointsY[1]
                        + 3*t*(1-t)*(1-t)*controlPointsY[2]
                        + t*t*t*controlPointsY[3];
    }

    g_BezierCurves["cubic"]["x"] = curvePointsX;
    g_BezierCurves["cubic"]["y"] = curvePointsY;
}

void DrawVirtualObject(const char* object_name) {
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    glBindVertexArray(0);
}

void LoadShadersFromFiles() {
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if (program_id != 0) glDeleteProgram(program_id);

    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    model_uniform           = glGetUniformLocation(program_id, "model");
    view_uniform            = glGetUniformLocation(program_id, "view");
    projection_uniform      = glGetUniformLocation(program_id, "projection");
    object_id_uniform       = glGetUniformLocation(program_id, "object_id");
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "PlatformTexture"), PLATFORM);
    glUniform1i(glGetUniformLocation(program_id, "PlayerTexture"), PLAYER);
    glUniform1i(glGetUniformLocation(program_id, "KeyTexture"), KEY);
    glUseProgram(0);
}

void PushMatrix(glm::mat4 M) {
    g_MatrixStack.push(M);
}

void PopMatrix(glm::mat4& M) {
    if (g_MatrixStack.empty()) {
        M = Matrix_Identity();
    } else {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

void ComputeNormals(ObjModel* model) {
    if (!model->attrib.normals.empty()) return;

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape) {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle) {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex) {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex) {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize(3*num_vertices);

    for (size_t i = 0; i < vertex_normals.size(); ++i) {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

GLuint BuildPlatform() {
    GLfloat cube_model_coefficients[] = {
        -0.5f,  0.5f,  0.5f, 1.0f,
        -0.5f, -0.5f,  0.5f, 1.0f,
         0.5f, -0.5f,  0.5f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f,
         0.5f, -0.5f, -0.5f, 1.0f,
         0.5f,  0.5f, -0.5f, 1.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
         1.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  1.0f,  0.0f, 1.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  0.0f,  1.0f, 1.0f,
    };

    GLuint VBO_cube_model_coefficients_id;
    glGenBuffers(1, &VBO_cube_model_coefficients_id);

    GLuint cube_vertex_array_object_id;
    glGenVertexArrays(1, &cube_vertex_array_object_id);

    glBindVertexArray(cube_vertex_array_object_id);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube_model_coefficients_id);

    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_model_coefficients), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_model_coefficients), cube_model_coefficients);

    GLuint location = 0;
    GLint number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(location);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLfloat cube_color_coefficients[] = {
        1.0f, 0.5f, 0.0f,
        1.0f, 0.5f, 0.0f,
        0.0f, 0.5f, 1.0f,
        0.0f, 0.5f, 1.0f,
        1.0f, 0.5f, 0.0f,
        1.0f, 0.5f, 0.0f,
        0.0f, 0.5f, 1.0f,
        0.0f, 0.5f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };
    GLuint VBO_cube_color_coefficients_id;
    glGenBuffers(1, &VBO_cube_color_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube_color_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_color_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_color_coefficients), cube_color_coefficients);
    location = 3;
    number_of_dimensions = 3;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices[] = {
        0, 1, 2,
        7, 6, 5,
        3, 2, 6,
        4, 0, 3,
        4, 5, 1,
        1, 5, 6,
        0, 2, 3,
        7, 5, 4,
        3, 6, 7,
        4, 3, 7,
        4, 1, 0,
        1, 6, 2,
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        0, 4,
        4, 7,
        7, 6,
        6, 2,
        6, 5,
        5, 4,
        5, 1,
        7, 3,
        8 , 9 ,
        10, 11,
        12, 13
    };

    SceneObject cube_faces;
    cube_faces.name = "Cubo (faces coloridas)";
    cube_faces.first_index = 0;
    cube_faces.num_indices = 36;
    cube_faces.rendering_mode = GL_TRIANGLES;
    g_VirtualScene["cube_faces"] = cube_faces;

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glBindVertexArray(0);

    return cube_vertex_array_object_id;
}

void LoadAllObjFiles() {
    ObjModel platform("../../data/platform.obj");
    ComputeNormals(&platform);
    BuildTrianglesAndAddToVirtualScene(&platform);

    ObjModel player("../../data/player.obj");
    ComputeNormals(&player);
    BuildTrianglesAndAddToVirtualScene(&player);

    ObjModel key("../../data/key.obj");
    ComputeNormals(&key);
    BuildTrianglesAndAddToVirtualScene(&key);
}

void BuildTrianglesAndAddToVirtualScene(ObjModel* model) {
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back(vx);
                model_coefficients.push_back(vy);
                model_coefficients.push_back(vz);
                model_coefficients.push_back(1.0f);

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if (idx.normal_index != -1) {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back(nx);
                    normal_coefficients.push_back(ny);
                    normal_coefficients.push_back(nz);
                    normal_coefficients.push_back(0.0f);
                }

                if (idx.texcoord_index != -1) {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index;
        theobject.num_indices    = last_index - first_index + 1;
        theobject.rendering_mode = GL_TRIANGLES;
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0;
    GLint  number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!normal_coefficients.empty()) {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1;
        number_of_dimensions = 4;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!texture_coefficients.empty()) {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2;
        number_of_dimensions = 2;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    glBindVertexArray(0);
}

GLuint LoadShader_Vertex(const char* filename) {
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    LoadShader(filename, vertex_shader_id);

    return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char* filename) {
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    LoadShader(filename, fragment_shader_id);

    return fragment_shader_id;
}

void LoadTextureImage(const char* filename) {
    printf("Carregando imagem \"%s\"... ", filename);

    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if (data == NULL) {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);


    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

void LoadShader(const char* filename, GLuint shader_id) {
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if (log_length != 0) {
        std::string  output;

        if (!compiled_ok) {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        } else {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    delete [] log;
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id) {
    GLuint program_id = glCreateProgram();

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    glLinkProgram(program_id);

    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    if (linked_ok == GL_FALSE) {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}


void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        g_LeftMouseButtonPressed = false;
    }
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!g_LeftMouseButtonPressed) return;

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.003f*dx;
    g_CameraPhi += 0.003f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax) g_CameraPhi = phimax;
    if (g_CameraPhi < phimin) g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    g_CameraDistance -= 0.1f*yoffset;

    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod) {
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);

    // Pressionar ESC fecha a janela onde roda a aplicação
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    // Pressionar R recarrega os shaders
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Pressionar W faz o jogador andar para frente
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            g_WPressed = true;
        } else if (action == GLFW_RELEASE) {
            g_WPressed = false;
        }
    }

    // Pressionar S faz o jogador andar para trás
    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            g_SPressed = true;
        } else if (action == GLFW_RELEASE) {
            g_SPressed = false;
        }
    }

    // Pressionar A faz o jogador andar para esquerda
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            g_APressed = true;
        } else if (action == GLFW_RELEASE) {
            g_APressed = false;
        }
    }

    // Pressionar D faz o jogador andar para direita
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            g_DPressed = true;
        } else if (action == GLFW_RELEASE) {
            g_DPressed = false;
        }
    }

    // Pressionar a barra de espaço faz o jogador pular
    if (key == GLFW_KEY_SPACE && g_CurrentTime - g_LastTimeJumped > 0.5) {
        g_LastTimeJumped = (float)glfwGetTime();
        g_SpacebarPressed = true;
    } else {
        g_SpacebarPressed = false;
    }

    if (key == GLFW_KEY_L) {
        if (action == GLFW_PRESS) {
            g_PlayerPosition = glm::vec4(0.0f, 3.0f, 0.0f, 1.0f);
            g_NewPlayerPosition = glm::vec4(0.0f, 3.0f, 0.0f, 1.0f);
        } 
    }
}

void ErrorCallback(int error, const char* description) {
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

