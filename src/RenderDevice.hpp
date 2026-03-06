#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

// Thin abstraction over raw OpenGL calls.
// Tracks currently bound shader and textures to skip redundant driver calls.
class RenderDevice {
public:
    void clear(float r, float g, float b, float a = 1.0f) {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void setViewport(int x, int y, int w, int h) {
        glViewport(x, y, w, h);
    }

    void setDepthTest(bool enable) {
        if (enable) glEnable(GL_DEPTH_TEST);
        else        glDisable(GL_DEPTH_TEST);
    }

    void setPolygonMode(GLenum face, GLenum mode) {
        glPolygonMode(face, mode);
    }

    // State-cached — skips glUseProgram if this shader is already active.
    void bindShader(const Shader& shader) {
        if (shader.id == boundShaderID_) return;
        glUseProgram(shader.id);
        boundShaderID_ = shader.id;
    }

    // State-cached per texture unit — skips the driver call if already bound.
    void bindTexture(const Texture& tex, GLenum unit = GL_TEXTURE0) {
        const int slot = static_cast<int>(unit - GL_TEXTURE0);
        if (tex.id == boundTextureIDs_[slot]) return;
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        boundTextureIDs_[slot] = tex.id;
    }

    // Uniform setters — forward to the currently bound shader.
    void setInt  (const char* name, int v)               { glUniform1i (loc(name), v); }
    void setFloat(const char* name, float v)             { glUniform1f (loc(name), v); }
    void setVec3 (const char* name, const glm::vec3& v)  { glUniform3fv(loc(name), 1, glm::value_ptr(v)); }
    void setMat4 (const char* name, const glm::mat4& m)  { glUniformMatrix4fv(loc(name), 1, GL_FALSE, glm::value_ptr(m)); }

    void drawMesh(const Mesh& mesh) { mesh.draw(); }

private:
    GLuint boundShaderID_      = 0;
    GLuint boundTextureIDs_[8] = {};

    GLint loc(const char* name) const {
        return glGetUniformLocation(boundShaderID_, name);
    }
};
