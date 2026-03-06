#pragma once
#include <map>
#include <string>
#include "Texture.hpp"

// Loads each texture path once and returns a stable const reference.
// std::map is used so insertions never invalidate references to existing elements.
class TextureCache {
public:
    TextureCache() = default;
    TextureCache(const TextureCache&)            = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    const Texture& get(const std::string& path, Texture::Wrap wrap = Texture::Wrap::Repeat) {
        auto it = cache_.find(path);
        if (it != cache_.end()) return it->second;
        auto [ins, _] = cache_.emplace(path, Texture::load(path.c_str(), wrap));
        return ins->second;
    }

private:
    std::map<std::string, Texture> cache_;
};
