#pragma once
#include <map>
#include <string>
#include "Mesh.hpp"

// Loads each FBX path once and returns a stable const reference.
// std::map is used (not unordered_map) because map insertions never
// invalidate references to existing elements, keeping all Mesh& safe.
class MeshCache {
public:
    MeshCache() = default;
    MeshCache(const MeshCache&)            = delete;
    MeshCache& operator=(const MeshCache&) = delete;

    const Mesh& get(const std::string& path) {
        auto it = cache_.find(path);
        if (it != cache_.end()) return it->second;
        auto [ins, _] = cache_.emplace(path, Mesh::load(path.c_str()));
        return ins->second;
    }

private:
    std::map<std::string, Mesh> cache_;
};
