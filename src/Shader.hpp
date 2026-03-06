#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

class Shader {
public:
    GLuint id = 0;

    // Construct from GLSL source strings.
    Shader(const char* vertSrc, const char* fragSrc) {
        GLuint vs = compile(GL_VERTEX_SHADER,   vertSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    // Construct from on-disk GLSL files.
    static Shader fromFiles(const char* vertPath, const char* fragPath) {
        std::string vs = readFile(vertPath);
        std::string fs = readFile(fragPath);
        return Shader(vs.c_str(), fs.c_str());
    }

    ~Shader() { if (id) glDeleteProgram(id); }

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept : id(o.id) { o.id = 0; }

    void use() const { glUseProgram(id); }

    void setInt  (const char* name, int v)               const { glUniform1i (glGetUniformLocation(id, name), v); }
    void setFloat(const char* name, float v)             const { glUniform1f (glGetUniformLocation(id, name), v); }
    void setVec3 (const char* name, const glm::vec3& v)  const { glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v)); }
    void setMat4 (const char* name, const glm::mat4& m)  const {
        glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(m));
    }

private:
    static std::string readFile(const char* path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "Shader: cannot open '" << path << "'\n";
            return "";
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "Shader compile error: " << log << '\n';
        }
        return s;
    }
};
