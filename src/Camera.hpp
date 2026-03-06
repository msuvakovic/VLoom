#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

class Camera {
public:
    float azimuth   = 0.0f;
    float elevation = 0.0f;
    float mouseSens = 0.002f;

    // Called from the GLFW cursor pos callback — accumulates every delta.
    // Safe to call whether locked or not; ignores events when unlocked.
    void onCursorPos(double x, double y) {
        if (!locked_) return;
        if (firstMouse_) {
            lastX_      = x;
            lastY_      = y;
            firstMouse_ = false;
            return;
        }
        azimuth   -= float(x - lastX_) * mouseSens;
        elevation -= float(y - lastY_) * mouseSens;
        elevation  = glm::clamp(elevation, glm::radians(-89.0f), glm::radians(89.0f));
        lastX_ = x;
        lastY_ = y;
    }

    // Call whenever cursor lock state changes.
    // Resets the first-mouse guard on lock so re-locking never causes a jump.
    void setLocked(bool locked) {
        locked_ = locked;
        if (locked_) firstMouse_ = true;
    }

    bool isLocked() const { return locked_; }

    glm::vec3 lookDir() const {
        return glm::vec3(
            -sinf(azimuth) * cosf(elevation),
             sinf(elevation),
            -cosf(azimuth) * cosf(elevation)
        );
    }

private:
    bool   locked_     = true;  // mirrors cursorLocked in AppState
    bool   firstMouse_ = true;
    double lastX_      = 0.0;
    double lastY_      = 0.0;
};
