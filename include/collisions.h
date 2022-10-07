#ifndef _COLLISIONS_H
#define _COLLISIONS_H

#include <cstdio>
#include <cstdlib>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bits/stdc++.h>

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

typedef struct spawnAttr{
    glm::vec3 spawnPos;
    glm::vec3 spawnVec;
}SPAWNATTR;

bool CheckCollision(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition);
std::map<std::string, bool> CheckCollisionLevel2(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition);
std::map<std::string, bool> CheckKeyCollision(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition);
bool CheckBulletCollision(SceneObject player, glm::vec4 g_NewPlayerPosition);
#endif //_COLLISIONS_H
