// Headers das bibliotecas OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers de C++
#include <bits/stdc++.h>

bool CheckCollision(SceneObject obj1, SceneObject obj2) {
    bool collisionX = obj1.bbox_max >= obj2.bbox_min && obj2.bbox_max >= obj1.bbox_min;
    std::cout << collisionX;
    return collisionX;
}
