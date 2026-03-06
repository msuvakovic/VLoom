#pragma once
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

class Texture {
public:
    GLuint id = 0;

    enum class Wrap { Repeat, Clamp, Mirror };

    static Texture load(const char* path, Wrap wrap = Wrap::Repeat) {
        stbi_set_flip_vertically_on_load(true);
        int w, h, ch;
        unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
        if (!data) { std::cerr << "Failed to load texture: " << path << '\n'; return {}; }
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        GLint wrapGL = (wrap == Wrap::Clamp)  ? GL_CLAMP_TO_EDGE
                     : (wrap == Wrap::Mirror) ? GL_MIRRORED_REPEAT
                     :                          GL_REPEAT;
        Texture t;
        glGenTextures(1, &t.id);
        glBindTexture(GL_TEXTURE_2D, t.id);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     wrapGL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     wrapGL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return t;
    }

    ~Texture() { if (id) glDeleteTextures(1, &id); }

    Texture() = default;
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& o) noexcept : id(o.id) { o.id = 0; }
    Texture& operator=(Texture&& o) noexcept { if (id) glDeleteTextures(1, &id); id = o.id; o.id = 0; return *this; }

    void bind(GLenum unit = GL_TEXTURE0) const {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    bool isValid() const { return id != 0; }
};
