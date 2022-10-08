#ifndef _COLLISIONS_H
#define _COLLISIONS_H

#include <cstdio>
#include <cstdlib>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bits/stdc++.h>

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

// Estrutura para definir a posição da bala de canhão
typedef struct spawnAttr{
    glm::vec3 spawnPos;
    glm::vec3 spawnVec;
}SPAWNATTR;

std::pair<std::map<std::string, bool>, bool> CheckCollision(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition, bool touchedGround);
std::pair<std::map<std::string, bool>, bool> CheckCollisionLevel2(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition, bool touchedGround);
std::map<std::string, bool> CheckKeyCollision(SceneObject player, std::vector<SceneObject> g_KeyHitBoxes, glm::vec4 g_NewPlayerPosition);
bool CheckBulletCollision(SceneObject player, glm::vec4 g_NewPlayerPosition, std::vector<SPAWNATTR> bulletPosition, glm::vec4 camera_position_c);
#endif //_COLLISIONS_H
