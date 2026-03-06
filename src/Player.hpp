#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include "SceneObject.hpp"

class Player {
public:
    glm::vec3 position;
    float     verticalVelocity = 0.0f;
    bool      isGrounded       = false;
    bool      lastColliding    = false;

    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float JUMP_VEL   = 5.0f;
    static constexpr float GRAVITY    = 9.8f;
    static constexpr float RADIUS     = 0.3f;
    static constexpr float HEIGHT     = 1.8f;
    static constexpr float EYE_HEIGHT = 1.6f;
    static constexpr float FALL_LIMIT = -20.0f;

    explicit Player(glm::vec3 spawnPos) : position(spawnPos), spawnPos_(spawnPos) {}

    // allowMove = false when ImGui owns the keyboard (cursor unlocked + WantCaptureKeyboard).
    // Gravity, collision, and respawn always run so the player doesn't freeze in midair.
    void update(GLFWwindow* win, float dt, float azimuth, const SceneObject& platform, bool allowMove = true) {
        auto [platMin, platMax] = platform.worldAABB();
        float platformTopY = platMax.y;

        glm::vec3 forward(-sinf(azimuth), 0.0f, -cosf(azimuth));
        glm::vec3 right(   cosf(azimuth), 0.0f, -sinf(azimuth));

        glm::vec3 oldPos = position;
        if (allowMove) {
            if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) position += forward * MOVE_SPEED * dt;
            if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) position -= forward * MOVE_SPEED * dt;
            if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) position -= right   * MOVE_SPEED * dt;
            if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) position += right   * MOVE_SPEED * dt;
        }

        // Horizontal side-collision: test at old Y to avoid vertical motion triggering it
        glm::vec3 testH(position.x, oldPos.y + 0.1f, position.z);
        lastColliding = aabbOverlap(testH, platMin, platMax);
        if (lastColliding) { position.x = oldPos.x; position.z = oldPos.z; }

        if (allowMove && glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
            verticalVelocity = JUMP_VEL;
            isGrounded = false;
        }

        verticalVelocity -= GRAVITY * dt;
        position.y       += verticalVelocity * dt;
        isGrounded        = false;

        // Land on the platform's top face within its XZ footprint
        if (position.y <= platformTopY &&
            position.x + RADIUS > platMin.x && position.x - RADIUS < platMax.x &&
            position.z + RADIUS > platMin.z && position.z - RADIUS < platMax.z)
        {
            position.y       = platformTopY;
            verticalVelocity = 0.0f;
            isGrounded       = true;
        }

        if (position.y < FALL_LIMIT) {
            position         = spawnPos_;
            verticalVelocity = 0.0f;
        }
    }

    glm::vec3 eyePosition() const { return position + glm::vec3(0.0f, EYE_HEIGHT, 0.0f); }

private:
    glm::vec3 spawnPos_;

    static bool aabbOverlap(glm::vec3 pos, glm::vec3 wMin, glm::vec3 wMax) {
        glm::vec3 pMin = pos + glm::vec3(-RADIUS, 0.0f,   -RADIUS);
        glm::vec3 pMax = pos + glm::vec3( RADIUS,  HEIGHT,  RADIUS);
        return pMin.x <= wMax.x && pMax.x >= wMin.x &&
               pMin.y <= wMax.y && pMax.y >= wMin.y &&
               pMin.z <= wMax.z && pMax.z >= wMin.z;
    }
};
