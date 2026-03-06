#pragma once
#include <string>
#include <cfloat>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.hpp"
#include "Texture.hpp"
#include "RenderDevice.hpp"

struct SceneObject {
    std::string    label;
    std::string    meshPath;           // basename relative to exe, e.g. "ramp1.fbx"
    std::string    texPath;            // basename relative to exe, e.g. "bricks_color.jpg"
    const Mesh*    mesh = nullptr;     // non-owning; points into MeshCache
    const Texture* tex  = nullptr;     // non-owning; points into TextureCache
    glm::vec3   pos      = glm::vec3(0.0f);
    glm::vec3   rot      = glm::vec3(0.0f); // Euler degrees, YXZ order
    glm::vec3   scale    = glm::vec3(1.0f);
    float       texTile  = 1.0f;
    glm::mat4   transform = glm::mat4(1.0f);

    SceneObject() = default;
    SceneObject(const SceneObject&)            = delete;
    SceneObject& operator=(const SceneObject&) = delete;
    SceneObject(SceneObject&&)                 = default;
    SceneObject& operator=(SceneObject&&)      = default;

    void rebuildTransform() {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), pos);
        t = glm::rotate(t, glm::radians(rot.y), glm::vec3(0,1,0));
        t = glm::rotate(t, glm::radians(rot.x), glm::vec3(1,0,0));
        t = glm::rotate(t, glm::radians(rot.z), glm::vec3(0,0,1));
        t = glm::scale(t, scale);
        transform = t;
    }

    // World-space AABB derived from transforming all 8 local corners.
    std::pair<glm::vec3, glm::vec3> worldAABB() const {
        glm::vec3 wMin( FLT_MAX), wMax(-FLT_MAX);
        if (!mesh) return {wMin, wMax};
        for (int i = 0; i < 8; ++i) {
            glm::vec3 c(
                (i & 1) ? mesh->boundsMax.x : mesh->boundsMin.x,
                (i & 2) ? mesh->boundsMax.y : mesh->boundsMin.y,
                (i & 4) ? mesh->boundsMax.z : mesh->boundsMin.z);
            glm::vec3 wc = glm::vec3(transform * glm::vec4(c, 1.0f));
            wMin = glm::min(wMin, wc);
            wMax = glm::max(wMax, wc);
        }
        return {wMin, wMax};
    }

    void draw(RenderDevice& rd) const {
        if (!mesh || !mesh->vao) return;
        if (tex && tex->isValid()) rd.bindTexture(*tex);
        rd.setFloat("uTileSize", texTile);
        rd.setMat4("u_Model", transform);
        rd.drawMesh(*mesh);
    }
};
