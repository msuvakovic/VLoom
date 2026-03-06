#pragma once
#include <glm/glm.hpp>
#include <cfloat>
#include <utility>
#include "Mesh.hpp"
#include "RenderDevice.hpp"

class Entity {
public:
    const Mesh& mesh;
    glm::mat4   transform;

    Entity(const Mesh& m, glm::mat4 t) : mesh(m), transform(std::move(t)) {}

    void draw(RenderDevice& rd) const {
        rd.setMat4("u_Model", transform);
        rd.drawMesh(mesh);
    }

    // World-space AABB derived from transforming all 8 local corners.
    std::pair<glm::vec3, glm::vec3> worldAABB() const {
        glm::vec3 wMin( FLT_MAX), wMax(-FLT_MAX);
        for (int i = 0; i < 8; ++i) {
            glm::vec3 c(
                (i & 1) ? mesh.boundsMax.x : mesh.boundsMin.x,
                (i & 2) ? mesh.boundsMax.y : mesh.boundsMin.y,
                (i & 4) ? mesh.boundsMax.z : mesh.boundsMin.z);
            glm::vec3 wc = glm::vec3(transform * glm::vec4(c, 1.0f));
            wMin = glm::min(wMin, wc);
            wMax = glm::max(wMax, wc);
        }
        return {wMin, wMax};
    }
};
