#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <iostream>
#include <cfloat>

class Mesh {
public:
    GLuint    vao = 0, vbo = 0, ebo = 0;
    GLsizei   indexCount = 0;
    glm::vec3 boundsMin  = glm::vec3( FLT_MAX);
    glm::vec3 boundsMax  = glm::vec3(-FLT_MAX);

    Mesh() = default;

    ~Mesh() {
        if (vao) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }
    }

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& o) noexcept
        : vao(o.vao), vbo(o.vbo), ebo(o.ebo),
          indexCount(o.indexCount),
          boundsMin(o.boundsMin), boundsMax(o.boundsMax)
    { o.vao = o.vbo = o.ebo = 0; }

    Mesh& operator=(Mesh&& o) noexcept {
        if (this != &o) {
            if (vao) { glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo); }
            vao = o.vao; vbo = o.vbo; ebo = o.ebo;
            indexCount = o.indexCount;
            boundsMin  = o.boundsMin;
            boundsMax  = o.boundsMax;
            o.vao = o.vbo = o.ebo = 0;
        }
        return *this;
    }

    static Mesh load(const char* path) {
        Assimp::Importer imp;
        const aiScene* scene = imp.ReadFile(path,
            aiProcess_Triangulate        |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals   |
            aiProcess_FlipUVs);

        if (!scene || !scene->mNumMeshes) {
            std::cerr << "Assimp: " << imp.GetErrorString() << '\n';
            return {};
        }

        std::vector<float>        verts;
        std::vector<unsigned int> indices;
        unsigned int indexOffset = 0;
        Mesh result;

        for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
            aiMesh* mesh = scene->mMeshes[m];

            for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
                float x = mesh->mVertices[i].x;
                float y = mesh->mVertices[i].y;
                float z = mesh->mVertices[i].z;
                verts.push_back(x); verts.push_back(y); verts.push_back(z);

                glm::vec3 p(x, y, z);
                result.boundsMin = glm::min(result.boundsMin, p);
                result.boundsMax = glm::max(result.boundsMax, p);

                if (mesh->HasNormals()) {
                    verts.push_back(mesh->mNormals[i].x);
                    verts.push_back(mesh->mNormals[i].y);
                    verts.push_back(mesh->mNormals[i].z);
                } else {
                    verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
                }

                if (mesh->mTextureCoords[0]) {
                    verts.push_back(mesh->mTextureCoords[0][i].x);
                    verts.push_back(mesh->mTextureCoords[0][i].y);
                } else {
                    verts.push_back(x * 0.25f);
                    verts.push_back(z * 0.25f);
                }
            }

            for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace& f = mesh->mFaces[i];
                for (unsigned j = 0; j < f.mNumIndices; ++j)
                    indices.push_back(indexOffset + f.mIndices[j]);
            }
            indexOffset += mesh->mNumVertices;
        }

        std::cout << "Loaded " << scene->mNumMeshes << " mesh(es), "
                  << indexOffset << " verts, " << indices.size() / 3 << " tris\n";

        result.indexCount = (GLsizei)indices.size();

        glGenVertexArrays(1, &result.vao);
        glGenBuffers(1, &result.vbo);
        glGenBuffers(1, &result.ebo);

        glBindVertexArray(result.vao);
        glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        return result;
    }

    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
};
