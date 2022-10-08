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
#include <iostream>
#include <set>
#include <utility>
// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

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
#include "collisions.h"
float PI = 3.14159265359f;
// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
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

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles();
void LoadShadersFromFilesLevel2();  // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void LoadAllObjFiles();
glm::mat4 PlatformBaseModel();
void ApplyPlataformTransformation();
void Draw(const char* objectName, glm::mat4 model, int objectNumber);
void DrawLevel1();
void MovePlayer(glm::vec4 camera_view_vector, glm::vec4 camera_up_vector, float delta_t);

// Funções relacionadas às texturas
void LoadTextureImage(const char* filename);
void LoadAllTextures();

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem
float g_CameraX = 0.0f;
float g_CameraY = 10.0f;
float g_CameraZ = 0.0f;

// Posição atual do jogador e possível posição futura
glm::vec4 g_PlayerPosition;
glm::vec4 g_NewPlayerPosition = glm::vec4(g_CameraX, g_CameraY, g_CameraZ, 1.0f);;

glm::vec4 camera_position_c;

bool isMovingRight = false;
bool isMovingForward = false;
bool isMovingBackward = false;
bool isMovingLeft = false;
// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = false;

bool touchedGround = false;

float oldAngle = 0.0f;
// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint camera_view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Variáveis que controlam a movimentação da câmera
bool g_WPressed = false; // Andar para frente
bool g_SPressed = false; // Andar para trás
bool g_APressed = false; // Andar para esquerda
bool g_DPressed = false; // Andar para direita
bool g_SpacebarPressed = false; // Pular

// IDs dos objetos
#define PLATFORM 0
#define PLAYER 1
#define KEY 2
#define CANNON 3

// Valores de tempo
float g_CurrentTime = 0;
float g_LastTimeJumped = 0;


float cameraSpeed = 1.0f;
float delta_t;

glm::vec4 turretVector = glm::vec4(1.0f,0.0f,0.0f,0.0f);
glm::vec3 turrentPosition = glm::vec3(8.0f,3.0f,4.0f);

glm::vec4 turretVector2 = glm::vec4(1.0f,0.0f,0.0f,0.0f);
glm::vec3 turrentPosition2 = glm::vec3(-20.0f,6.0f,20.0f);

glm::vec4 turretVector3 = glm::vec4(1.0f,0.0f,0.0f,0.0f);
glm::vec3 turrentPosition3 = glm::vec3(12.0f, 9.0f, 16.0f);

// Estrutura para calcular a hitbox dos objetos em cena
typedef struct cameraPlayer{
        glm::vec3 bottom_left_back;
        glm::vec3 up_right_front;
}CameraPlayer;

CameraPlayer player;

// Calcula a hitbox dos objetos em cena
void CalculateHitBox(std::string objName, float x, float y, float z, float scaleFactorX = 1.0, float scaleFactorY = 1.0, float scaleFactorZ = 1.0);
// Vetores onde são guardados as hitboxes usados para calcular as colisões
std::vector<SceneObject> g_HitBoxes;
std::vector<SceneObject> g_KeyHitBoxes;

// Função que carrega todas as texturas da fase 1
void LoadAllTextures() {
    LoadTextureImage("../../data/level2-platform-texture.jpg");
    LoadTextureImage("../../data/level2-key-texture.jpg");
    LoadTextureImage("../../data/10471_Laser_Turret_v1_Diffuse.jpg");
}

// Função que carrega os objetos da fase 1
void LoadAllObjFiles() {
    ObjModel platform("../../data/platform.obj");
    ComputeNormals(&platform);
    BuildTrianglesAndAddToVirtualScene(&platform);

    ObjModel key("../../data/key.obj");
    ComputeNormals(&key);
    BuildTrianglesAndAddToVirtualScene(&key);

    ObjModel cannon("../../data/turret.obj");
    ComputeNormals(&cannon);
    BuildTrianglesAndAddToVirtualScene(&cannon);

    ObjModel sphere("../../data/sphere.obj");
    ComputeNormals(&sphere);
    BuildTrianglesAndAddToVirtualScene(&sphere);
}

// Função que carrega as texturas da fase 2
void LoadAllTexturesLevel2(){
    LoadTextureImage("../../data/level2-player-texture.jpg");
}

// Função que carrega os objetos da fase 2
void LoadAllObjFilesLevel2() {
    ObjModel player("../../data/player.obj");
    ComputeNormals(&player);
    BuildTrianglesAndAddToVirtualScene(&player);
}

void initializeBezierCurves();
void buildBezierCurveXTranslation();
void buildBezierCurveYTranslation();
void buildBezierCurveDegree2();
void buildBezierCurveDegree3();
std::map<std::string, float> retrieveBezierCurvePoint(std::string curveName);
std::map<std::string, std::map<std::string, std::array<float, 200>>> g_BezierCurves;

glm::mat4 PlatformBaseModel() {
    glm::mat4 model = Matrix_Identity() * Matrix_Translate(0.0f, 0.0f, 0.0f);
    return model;
}

glm::mat4 CannonBaseModel(){
    glm::mat4 model = Matrix_Identity() * Matrix_Translate(0.0f, 0.0f, 0.0f);
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

// Variável que controla a troca de fases
bool changeLevel = false;

float spherePositionX = 0.0f;

std::vector<SPAWNATTR> bulletPosition;

// Função que desenha as plataformas e canhões da fase 1
void ApplyPlataformTransformation() {
    CalculateHitBox("platform", 0.0f, 2.0f, 0.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    glm::mat4 model = PlatformBaseModel() * Matrix_Translate(0.0f, 2.0f, 0.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 0.0f, 2.0f, 4.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(0.0f, 2.0f, 4.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 0.0f, 2.0f, 8.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(0.0f, 2.0f, 8.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 2.0f, 8.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 2.0f, 8.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -8.0f, 3.0f, 8.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-8.0f, 3.0f, 8.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -12.0f, 4.0f, 8.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-12.0f, 4.0f, 8.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -12.0f, 4.0f, 12.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-12.0f, 4.0f, 12.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -12.0f, 5.0f, 16.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-12.0f, 5.0f, 16.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -12.0f, 5.0f, 20.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-12.0f, 5.0f, 20.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -12.0f, 5.0f, 24.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-12.0f, 5.0f, 24.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -12.0f, 5.0f, 28.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-12.0f, 5.0f, 28.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -8.0f, 6.0f, 28.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-8.0f, 6.0f, 28.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -8.0f, 6.0f, 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-8.0f, 6.0f, 32.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", -4.0f, 7.0f, 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 7.0f, 32.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 0.0f, 7.0f, 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(0.0f, 7.0f, 32.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 7.0f, 32.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 7.0f, 32.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 8.0f, 28.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 8.0f, 28.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 8.0f, 24.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 8.0f, 24.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 8.0f, 20.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 8.0f, 20.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 4.0f, 8.0f, 16.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(4.0f, 8.0f, 16.0f);
    Draw("platform", model, PLATFORM);

    CalculateHitBox("platform", 0.0f, 8.0f, 16.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(0.0f, 8.0f, 16.0f);
    Draw("platform", model, PLATFORM);


    //CalculateHitBox("platform", 12.0f, 9.0f, 16.0f);
    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(12.0f, 8.8f, 16.0f);
    Draw("platform", model, PLATFORM);

    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(-20.0f,5.8f,20.0f);
    Draw("platform", model, PLATFORM);

    g_HitBoxes.push_back(g_VirtualScene["platform"]);
    model = PlatformBaseModel() * Matrix_Translate(8.0f,2.8f,4.0f);
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

    CalculateHitBox("key", -4.0f, 9.75f, 15.0f);
    g_KeyHitBoxes.push_back(g_VirtualScene["key"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 9.75f, 15.0f) * Matrix_Rotate_Y(3.1415/2) * Matrix_Rotate_Z(3.1415/2) * Matrix_Scale(0.25, 0.25, 0.25);
    Draw("key", model, KEY);

    glm::vec3 vecPos = glm::vec3(camera_position_c.x,0.0f,camera_position_c.z);

    model = CannonBaseModel() * Matrix_Translate(turrentPosition.x,turrentPosition.y,turrentPosition.z)*Matrix_Scale(0.001f,0.001f,0.001f)*glm::inverse(glm::lookAt(glm::vec3(turrentPosition.x,0.0f,turrentPosition.z), vecPos, glm::vec3(0.0f,1.0f,0.0f)))*Matrix_Rotate_X(-3.1415f/2)*Matrix_Rotate_Z(-3.1415f);
    Draw("cannon",model,CANNON);

    glm::vec3 vecPos2 = glm::vec3(camera_position_c.x,0.0f,camera_position_c.z);

    model = CannonBaseModel() * Matrix_Translate(turrentPosition2.x,turrentPosition2.y,turrentPosition2.z)*Matrix_Scale(0.001f,0.001f,0.001f)*glm::inverse(glm::lookAt(glm::vec3(turrentPosition2.x,0.0f,turrentPosition2.z), vecPos2, glm::vec3(0.0f,1.0f,0.0f)))*Matrix_Rotate_X(-3.1415f/2)*Matrix_Rotate_Z(-3.1415f);
    Draw("cannon",model,CANNON);

    glm::vec3 vecPos3 = glm::vec3(camera_position_c.x,0.0f,camera_position_c.z);

    model = CannonBaseModel() * Matrix_Translate(turrentPosition3.x,turrentPosition3.y,turrentPosition3.z)*Matrix_Scale(0.001f,0.001f,0.001f)*glm::inverse(glm::lookAt(glm::vec3(turrentPosition3.x,0.0f,turrentPosition3.z), vecPos3, glm::vec3(0.0f,1.0f,0.0f)))*Matrix_Rotate_X(-3.1415f/2)*Matrix_Rotate_Z(-3.1415f);
    Draw("cannon",model,CANNON);


    for(auto bPos:bulletPosition){
        model = CannonBaseModel() * Matrix_Translate(bPos.spawnPos.x,bPos.spawnPos.y,bPos.spawnPos.z) * Matrix_Scale(0.1f,0.1f,0.1f);
        Draw("sphere",model,CANNON);
    }

}

// Função que desenha os objetos em tela
void Draw(const char* objectName, glm::mat4 model, int objectNumber) {
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, objectNumber);
    DrawVirtualObject(objectName);
}

void DrawLevel1() {
    ApplyPlataformTransformation();
}

// Função que move a bala do canhão
void MoveBullet(){
    for (unsigned int i = 0; i < bulletPosition.size(); i++) {
        bulletPosition[i].spawnPos = bulletPosition[i].spawnPos + bulletPosition[i].spawnVec*delta_t;
    }
}

// Variáveis usadas para tratar das curvas de Bézier
bool trocou = false;
bool invertAnimationDirection = false;

// Função que desenha as plataformas e canhões da fase 2
void ApplyPlataformTransformationLevel2() {
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
    g_KeyHitBoxes.push_back(g_VirtualScene["key"]);
    model = PlatformBaseModel() * Matrix_Translate(-4.0f, 9.75f, 15.0f) * Matrix_Rotate_Y(3.1415/2) * Matrix_Rotate_Z(3.1415/2) * Matrix_Scale(0.25, 0.25, 0.25);
    Draw("key", model, KEY);
}

void DrawLevel2() {
    ApplyPlataformTransformationLevel2();
}

// Recebe o nome de uma das curvas de Bézier e retona um ponto específico da curva
std::map<std::string, float> retrieveBezierCurvePoint(std::string curveName) {
    float auxTime = g_CurrentTime;
    int remainder200 = (int)(g_CurrentTime*100)%200;

    // Como nem sempre a variável remainder200 contém o valor 200, 
    // estabeleceu-se um limite de 194 para trocar a orientação da animação
    // dos objetos que se movem usando curvas de Bézier.
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

    // Ponto da curva retornado
    std::map<std::string, float> point;
    point["x"] = currentX;
    point["y"] = currentY;

    return point;
}

// Inicializar todas as curvas de Bézier antes de rodar o laço principal da aplicação
void initializeBezierCurves() {
    buildBezierCurveXTranslation();
    buildBezierCurveYTranslation();
    buildBezierCurveDegree2();
    buildBezierCurveDegree3();
}

// Curva de Bézier de grau 1 que se move apenas no eixo x
void buildBezierCurveXTranslation() {
    // Pontos de controle da curva no eixo x
    float controlPointsX[2];
    controlPointsX[0] = -2.0;
    controlPointsX[1] = 2.0;

    // Pontos de controler da curva no eixo y
    float controlPointsY[2];
    controlPointsY[0] = 0.0;
    controlPointsY[1] = 0.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    // Interpolação dos pontos da curva
    for (i = 0, t = 0; t <= 1; i++, t += 0.005) {
        curvePointsX[i] = controlPointsX[0] + t*(controlPointsX[1] - controlPointsX[0]);

        curvePointsY[i] = controlPointsY[0] + t*(controlPointsY[1] - controlPointsY[0]);
    }

    g_BezierCurves["x_translate"]["x"] = curvePointsX;
    g_BezierCurves["x_translate"]["y"] = curvePointsY;
}

// Curva de Bézier de grau 1 que se move apenas no eixo Y
void buildBezierCurveYTranslation() {
    // Pontos de controle da curva no eixo x
    float controlPointsX[2];
    controlPointsX[0] = 0.0;
    controlPointsX[1] = 0.0;

    // Pontos de controle da curva no eixo y
    float controlPointsY[2];
    controlPointsY[0] = -2.0;
    controlPointsY[1] = 2.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    // Interpolação dos pontos da curva
    for (i = 0, t = 0; t <= 1; i++, t += 0.005) {
        curvePointsX[i] = controlPointsX[0] + t*(controlPointsX[1] - controlPointsX[0]);

        curvePointsY[i] = controlPointsY[0] + t*(controlPointsY[1] - controlPointsY[0]);
    }

    g_BezierCurves["y_translate"]["x"] = curvePointsX;
    g_BezierCurves["y_translate"]["y"] = curvePointsY;
}

// Curva de Bézier de grau 2
void buildBezierCurveDegree2() {
    // Pontos de controle da curva no eixo x
    float controlPointsX[3];
    controlPointsX[0] = -1.0;
    controlPointsX[1] = 0.0;
    controlPointsX[2] = 1.0;

    // Pontos de controle da curva no eixo y
    float controlPointsY[3];
    controlPointsY[0] = -1.0;
    controlPointsY[1] = 1.0;
    controlPointsY[2] = -1.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    // Interpolação dos pontos de controle
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

// Curva de Bézier de grau 3
void buildBezierCurveDegree3() {
    // Pontos de controle da curva no eixo x
    float controlPointsX[4];
    controlPointsX[0] = 0.0;
    controlPointsX[1] = -3.0;
    controlPointsX[2] = 3.0;
    controlPointsX[3] = 0.0;

    // Pontos de controle da curva no eixo y
    float controlPointsY[4];
    controlPointsY[0] = 0.0;
    controlPointsY[1] = 3.0;
    controlPointsY[2] = 3.0;
    controlPointsY[3] = 0.0;

    std::array<float, 200> curvePointsX;
    std::array<float, 200> curvePointsY;

    int i = 0;
    float t = 0;
    // Interpolação dos pontos de controle
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

// Calcula a posição futura do jogador. Se houver uma colisão, o jogador não é movido
// Caso contrário, movemos o jogador
void MovePlayer(glm::vec4 camera_view_vector, glm::vec4 camera_up_vector, float delta_t) {
        // Vetor normalizado que aponta para onde a câmera está olhando. Esse vetor é usado
        // para mover o jogador na diração para onde a câmera está olhando.
        glm::vec4 camera_view_vector_n = normalize(camera_view_vector);
        camera_view_vector_n = glm::vec4(camera_view_vector_n.x*3, camera_view_vector_n.y*3, camera_view_vector_n.z*3, camera_view_vector_n.w);

        // Vetor que define a movimentação para trás e para frente (teclas W e S).
        glm::vec4 wsMoveDirection = glm::vec4(camera_view_vector_n.x, 0.0f, camera_view_vector_n.z, 0.0f);
        if(!changeLevel){
            if (g_WPressed) g_NewPlayerPosition = camera_position_c + wsMoveDirection * cameraSpeed * delta_t;
            if (g_SPressed) g_NewPlayerPosition = camera_position_c - wsMoveDirection * cameraSpeed * delta_t;
        }else{
            if (g_WPressed) g_NewPlayerPosition = g_PlayerPosition + wsMoveDirection * cameraSpeed * delta_t;
            if (g_SPressed) g_NewPlayerPosition = g_PlayerPosition - wsMoveDirection * cameraSpeed * delta_t;

        }

        // Vetor que define a movimentação lateral do jogador (teclas A e D).
        glm::vec4 sideVector = crossproduct(camera_view_vector_n, camera_up_vector);
        glm::vec4 adMoveDirection = glm::vec4(sideVector.x, 0.0f, sideVector.z, 0.0f);
        if(!changeLevel){
            if (g_APressed) g_NewPlayerPosition = camera_position_c - adMoveDirection * cameraSpeed * delta_t;
            if (g_DPressed) g_NewPlayerPosition = camera_position_c + adMoveDirection * cameraSpeed * delta_t;
        }else{
             if (g_APressed) g_NewPlayerPosition = g_PlayerPosition - adMoveDirection * cameraSpeed * delta_t;
            if (g_DPressed) g_NewPlayerPosition = g_PlayerPosition + adMoveDirection * cameraSpeed * delta_t;

        }

        // Movimentação do pulo do jogador
        if (g_SpacebarPressed) g_NewPlayerPosition = glm::vec4(
            g_NewPlayerPosition.x, 
            g_NewPlayerPosition.y + 0.15f, 
            g_NewPlayerPosition.z, 
            g_NewPlayerPosition.w
        );
}

// Gera as balas do canhão
void spawnBullet(glm::vec3 spawnPos, glm::vec3 spawnVec){
    SPAWNATTR sp;
    sp.spawnPos = spawnPos;
    sp.spawnVec = spawnVec;
    bulletPosition.push_back(sp);
}

// Reseta a fase quando o jogador é atingido por uma bala de canhão
void diedRoutine(){
    g_NewPlayerPosition = glm::vec4(0.0f,5.0f,0.0f,1.0f);
    bulletPosition.clear();
}

// Reseta a fase quando o jogador cai de uma plataforma
bool fell(){
    if(g_NewPlayerPosition.y < -1.0f){
        return true;
    }else
        return false;
}

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Seu Cartao - Seu Nome", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();



    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    LoadAllTextures();
    LoadAllObjFiles();

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float previous_time = (float)glfwGetTime();
    float current_time = (float)glfwGetTime();
    delta_t = current_time - previous_time;
    previous_time = current_time;

    //Centraliza e esconde o cursor
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);  

    float first_time = (float)glfwGetTime();
    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);


        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        float g_CameraTheta2 = g_CameraTheta;
        g_CameraTheta2 -= 3.141592f/2;
        
        camera_position_c  = g_NewPlayerPosition; // Ponto "c", centro da câmera

        //glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        //glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_view_vector = glm::vec4(x,y,z,0.0f);
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        DrawLevel1();
        MovePlayer(camera_view_vector, camera_up_vector, delta_t);
        std::pair<std::map<std::string, bool>, bool> collisions = CheckCollision(g_VirtualScene["player"], g_HitBoxes, g_NewPlayerPosition, touchedGround);
        if (!(collisions.first["x"] && collisions.first["y"] && collisions.first["z"])) {
            g_NewPlayerPosition = glm::vec4(g_NewPlayerPosition.x, g_NewPlayerPosition.y-(delta_t*cameraSpeed*3), g_NewPlayerPosition.z, g_NewPlayerPosition.w);
        }
        touchedGround = collisions.second;
        g_HitBoxes.clear();
        camera_position_c = g_NewPlayerPosition;

        // Gerando as balas de canhão
        if(current_time - first_time > 3){
            std::cout << "spawn bullet" << std::endl;
            glm::vec3 spawnVec = glm::vec3(camera_position_c.x -turrentPosition.x , camera_position_c.y -turrentPosition.y,camera_position_c.z - turrentPosition.z); 
            spawnBullet(turrentPosition,spawnVec);

            glm::vec3 spawnVec2 = glm::vec3(camera_position_c.x -turrentPosition2.x , camera_position_c.y -turrentPosition2.y,camera_position_c.z - turrentPosition2.z); 
            spawnBullet(turrentPosition2,spawnVec2);

            glm::vec3 spawnVec3 = glm::vec3(camera_position_c.x -turrentPosition3.x , camera_position_c.y -turrentPosition3.y,camera_position_c.z - turrentPosition3.z); 
            spawnBullet(turrentPosition3,spawnVec3);
            first_time = current_time; 
        }

        MoveBullet();
        if(CheckBulletCollision(g_VirtualScene["player"], g_NewPlayerPosition, bulletPosition, camera_position_c) || fell()){
            // std::cout << "DIED" << std::endl;
            diedRoutine();
        }

        std::map<std::string, bool> keycollisions = CheckKeyCollision(g_VirtualScene["player"], g_HitBoxes, g_NewPlayerPosition);
        if ((keycollisions["x"] && keycollisions["y"] && keycollisions["z"])) {
            //std::cout << "key collide" << std::endl;
            changeLevel = true;
        }
        current_time = glfwGetTime();
        delta_t = current_time - previous_time;
        previous_time = current_time;
        g_CurrentTime = glfwGetTime();

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -60.0f; // Posição do "far plane"

        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane); 

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        glUniform4f(camera_view_uniform,camera_view_vector.x,camera_view_vector.y,camera_view_vector.z,0.0f);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
        g_KeyHitBoxes.clear();

        if(changeLevel){
            break;
        }

    }

    LoadShadersFromFilesLevel2();
    LoadAllObjFilesLevel2();
    LoadAllTexturesLevel2();
    g_PlayerPosition = glm::vec4(0.0f, 3.0f, 0.0f, 1.0f);
    g_NewPlayerPosition = glm::vec4(0.0f, 3.0f, 0.0f, 1.0f);

    initializeBezierCurves();
    changeLevel = true;
    while (!glfwWindowShouldClose(window)) {
        // Inicialização
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program_id);

        // Variáveis para definir a posição da câmera look-at
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

        glm::mat4 projection;

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
        
        DrawLevel2();
        // Movemos o jogador
        MovePlayer(camera_view_vector, camera_up_vector, delta_t);
        // Verificamos se essa movimentação gerou alguma colisão
        std::pair<std::map<std::string, bool>, bool> collisions = CheckCollisionLevel2(g_VirtualScene["player"], g_HitBoxes, g_NewPlayerPosition, touchedGround);
        if (!(collisions.first["x"] && collisions.first["y"] && collisions.first["z"])) {
            g_NewPlayerPosition = glm::vec4(g_NewPlayerPosition.x, g_NewPlayerPosition.y-(delta_t*cameraSpeed*3), g_NewPlayerPosition.z, g_NewPlayerPosition.w);
        }
        // Atualizamos a variável que controla quando o jogador toca na plataforma
        touchedGround = collisions.second;
        // Limpamos as hitboxes. As hitboxes são calculadas baseadas no delta_t,
        // já que algumas plataformas se movem e sua hitbox muda de posição.
        g_HitBoxes.clear();
        g_KeyHitBoxes.clear();
        // Nova posição do jogador
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

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");
    camera_view_uniform     = glGetUniformLocation(program_id, "camera_view");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "PlatformTexture"), PLATFORM);
    glUniform1i(glGetUniformLocation(program_id, "KeyTexture"), KEY);
    glUniform1i(glGetUniformLocation(program_id, "PlayerTexture"), PLAYER);
    glUniform1i(glGetUniformLocation(program_id, "CannonTexture"), CANNON);
    glUseProgram(0);
}

void LoadShadersFromFilesLevel2() {
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex_level2.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment_level2.glsl");

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

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
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

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
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
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
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
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
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
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
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

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

   // if (g_LeftMouseButtonPressed)
    //{
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.008f*dx;
        g_CameraPhi   += 0.008f*dy;
    
        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;
    
        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;
    
        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
   // }

    if (g_RightMouseButtonPressed) {
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed) {
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

     // Se o usuário apertar a tecla W, move a câmera para frente
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            std::cout << "pressing w" << std::endl;
            isMovingForward = true;
            g_WPressed = true;
        } else if (action == GLFW_RELEASE) {
            isMovingForward = false;
            g_WPressed = false;
        }
    }

    // Se o usuário apertar a tecla A, move a câmera para a esquerda
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            isMovingLeft = true;
            g_APressed = true;
        } else if (action == GLFW_RELEASE) {
            isMovingLeft = false;
            g_APressed = false;
        }
    }

    // Se o usuário apertar a tecla S, move a câmera para trás
    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            isMovingBackward = true;
            g_SPressed = true;
        } else if (action == GLFW_RELEASE) {
            isMovingBackward = false;
            g_SPressed = false;
        }
    }

    // Se o usuário apertar a tecla D, move a câmera para a direita
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            isMovingRight = true;
            g_DPressed = true;
        } else if (action == GLFW_RELEASE) {
            isMovingRight = false;
            g_DPressed = false;
        }
    }

     // Pressionar a barra de espaço faz o jogador pular
    if (key == GLFW_KEY_SPACE && g_CurrentTime - g_LastTimeJumped > 0.5 && touchedGround) {
        g_LastTimeJumped = (float)glfwGetTime();
        g_SpacebarPressed = true;
    } else {
        g_SpacebarPressed = false;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98