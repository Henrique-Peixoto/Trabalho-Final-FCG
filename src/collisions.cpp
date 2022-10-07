// Headers das bibliotecas OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers de C++
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

// std::map<std::string, bool> CheckCollision(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition) {
//     bool collisionX = false;
//     bool collisionY = false;
//     bool collisionZ = false;
//     //SceneObject &player = g_VirtualScene["player"];

//     for (SceneObject platform : g_HitBoxes) {
//         player.bottom_left_back = glm::vec3(
//             g_NewPlayerPosition.x+player.bbox_min.x,
//             g_NewPlayerPosition.y+player.bbox_min.y,
//             g_NewPlayerPosition.z+player.bbox_min.z
//         );
//         player.up_right_front = glm::vec3(
//             g_NewPlayerPosition.x+player.bbox_max.x,
//             g_NewPlayerPosition.y+player.bbox_max.y,
//             g_NewPlayerPosition.z+player.bbox_max.z
//         );

//         collisionX = player.up_right_front.x >= platform.bottom_left_back.x
//                    && platform.up_right_front.x >= player.bottom_left_back.x;
//         collisionY = player.up_right_front.y >= platform.bottom_left_back.y
//                    && platform.up_right_front.y >= player.bottom_left_back.y;
//         collisionZ = player.up_right_front.z >= platform.bottom_left_back.z
//                    && platform.up_right_front.z >= player.bottom_left_back.z;

//        if (collisionX && collisionY && collisionZ) break;
//     }

//     std::map<std::string, bool> collisions;
//     collisions["x"] = collisionX;
//     collisions["y"] = collisionY;
//     collisions["z"] = collisionZ;

//     return collisions;
// }

std::map<std::string, bool> CheckCollision(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition) {
    bool collisionX = false;
    bool collisionY = false;
    bool collisionZ = false;
    //SceneObject &player = g_VirtualScene["player"];

    
    player.bottom_left_back = g_NewPlayerPosition + glm::vec4(-0.5f,-1.5f,-0.5f,1.0f);
    player.up_right_front = g_NewPlayerPosition + glm::vec4(0.5f,1.5f,0.5f,1.0f);

    for (SceneObject platform : g_HitBoxes) {

        collisionX = player.up_right_front.x >= platform.bottom_left_back.x
                   && platform.up_right_front.x >= player.bottom_left_back.x;
        collisionY = player.up_right_front.y >= platform.bottom_left_back.y
                   && platform.up_right_front.y >= player.bottom_left_back.y;
        collisionZ = player.up_right_front.z >= platform.bottom_left_back.z
                   && platform.up_right_front.z >= player.bottom_left_back.z;

       if (collisionX && collisionY && collisionZ) break;
    }
    if (collisionX && collisionY && collisionZ)
        touchedGround = true;
    else
        touchedGround = false;

    std::map<std::string, bool> collisions;
    collisions["x"] = collisionX;
    collisions["y"] = collisionY;
    collisions["z"] = collisionZ;

    return collisions;
}
std::map<std::string, bool> CheckCollisionLevel2(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition) {
    bool collisionX = false;
    bool collisionY = false;
    bool collisionZ = false;
    // SceneObject &player = g_VirtualScene["player"];

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
    if (collisionX && collisionY && collisionZ)
        touchedGround = true;
    else
        touchedGround = false;

    std::map<std::string, bool> collisions;
    collisions["x"] = collisionX;
    collisions["y"] = collisionY;
    collisions["z"] = collisionZ;

    return collisions;
}

std::map<std::string, bool> CheckKeyCollision(SceneObject player, std::vector<SceneObject> g_HitBoxes, glm::vec4 g_NewPlayerPosition) {
    bool collisionX = false;
    bool collisionY = false;
    bool collisionZ = false;
    //SceneObject &player = g_VirtualScene["player"];

    player.bottom_left_back = g_NewPlayerPosition + glm::vec4(-0.5f,-1.5f,-0.5f,1.0f);
    player.up_right_front = g_NewPlayerPosition + glm::vec4(0.5f,1.5f,0.5f,1.0f);

    for (SceneObject key : g_KeyHitBoxes) {

        collisionX = player.up_right_front.x >= key.bottom_left_back.x
                   && key.up_right_front.x >= player.bottom_left_back.x;
        collisionY = player.up_right_front.y >= key.bottom_left_back.y
                   && key.up_right_front.y >= player.bottom_left_back.y;
        collisionZ = player.up_right_front.z >= key.bottom_left_back.z
                   && key.up_right_front.z >= player.bottom_left_back.z;

       if (collisionX && collisionY && collisionZ) break;
    }


    std::map<std::string, bool> collisions;
    collisions["x"] = collisionX;
    collisions["y"] = collisionY;
    collisions["z"] = collisionZ;

    return collisions;
}

bool CheckBulletCollision(SceneObject player, glm::vec4 g_NewPlayerPosition) {
    player.bottom_left_back = g_NewPlayerPosition + glm::vec4(-0.5f,-1.5f,-0.5f,1.0f);
    player.up_right_front = g_NewPlayerPosition + glm::vec4(0.5f,1.5f,0.5f,1.0f);


    int idx = 0;
    bool colidiu = false;
    bool popFront = false;
    std::vector<SPAWNATTR> newBulletPosition;
    for(int i=0;i<bulletPosition.size();i++){
        SPAWNATTR bPos = bulletPosition[i];
        float sphereXDistance = abs(bPos.spawnPos.x - camera_position_c.x);
        float sphereYDistance = abs(bPos.spawnPos.y - camera_position_c.y);
        float sphereZDistance = abs(bPos.spawnPos.z - camera_position_c.z);

        
   
        float cornerDistance_sq = ((sphereXDistance) * (sphereXDistance)) +
                         ((sphereYDistance) * (sphereYDistance) +
                         ((sphereZDistance) * (sphereZDistance)));
        colidiu |= (cornerDistance_sq < (0.4f * 0.4f));
        if(cornerDistance_sq < 200.0f){
            newBulletPosition.push_back(bulletPosition[i]);
        }
    }
    bulletPosition = newBulletPosition;


    return colidiu;

}
