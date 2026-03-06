#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include "SceneObject.hpp"
#include "MeshCache.hpp"
#include "TextureCache.hpp"

// Simple INI-style scene format:
//   [object]
//   label=Ramp
//   mesh=ramp1.fbx
//   tex=bricks_color.jpg
//   pos=10 0 10
//   rot=0 0 0
//   scale=1 1 1
//   tile=1
//
// All mesh/tex values are basenames relative to the scene file's directory
// (which is always the same as the executable directory).

namespace SceneSerializer {

inline bool save(const std::string& filePath, const std::vector<SceneObject>& objects) {
    std::ofstream f(filePath);
    if (!f) { std::cerr << "Scene save failed: " << filePath << '\n'; return false; }

    for (const auto& obj : objects) {
        f << "[object]\n";
        f << "label=" << obj.label << '\n';
        f << "mesh="  << obj.meshPath << '\n';
        f << "tex="   << obj.texPath  << '\n';
        f << "pos="   << obj.pos.x   << ' ' << obj.pos.y   << ' ' << obj.pos.z   << '\n';
        f << "rot="   << obj.rot.x   << ' ' << obj.rot.y   << ' ' << obj.rot.z   << '\n';
        f << "scale=" << obj.scale.x << ' ' << obj.scale.y << ' ' << obj.scale.z << '\n';
        f << "tile="  << obj.texTile << '\n';
        f << '\n';
    }

    std::cout << "Scene saved: " << filePath << " (" << objects.size() << " objects)\n";
    return true;
}

inline bool load(const std::string& filePath,
                 MeshCache& meshCache, TextureCache& texCache,
                 std::vector<SceneObject>& objects) {
    std::ifstream f(filePath);
    if (!f) { std::cerr << "Scene load failed: " << filePath << '\n'; return false; }

    // Assets sit next to the scene file (= exe dir).
    std::filesystem::path dir = std::filesystem::path(filePath).parent_path();
    auto resolve = [&](const std::string& name) {
        return (dir / name).string();
    };

    // Accumulate fields for the current object, then flush into the vector.
    struct Pending {
        std::string label, meshPath, texPath;
        glm::vec3 pos{0.0f}, rot{0.0f}, scale{1.0f};
        float tile = 1.0f;
        bool active = false;
    } cur;

    auto flush = [&]() {
        if (!cur.active || cur.meshPath.empty()) return;
        SceneObject obj;
        obj.label    = cur.label;
        obj.meshPath = cur.meshPath;
        obj.texPath  = cur.texPath;
        obj.mesh     = &meshCache.get(resolve(cur.meshPath));
        if (!cur.texPath.empty())
            obj.tex = &texCache.get(resolve(cur.texPath));
        obj.pos     = cur.pos;
        obj.rot     = cur.rot;
        obj.scale   = cur.scale;
        obj.texTile = cur.tile;
        obj.rebuildTransform();
        objects.push_back(std::move(obj));
        cur = {};
    };

    objects.clear();
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line == "[object]") { flush(); cur.active = true; continue; }
        if (!cur.active) continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if      (key == "label") { cur.label    = val; }
        else if (key == "mesh")  { cur.meshPath = val; }
        else if (key == "tex")   { cur.texPath  = val; }
        else if (key == "tile")  { cur.tile = std::stof(val); }
        else if (key == "pos")   { std::istringstream ss(val); ss >> cur.pos.x   >> cur.pos.y   >> cur.pos.z;   }
        else if (key == "rot")   { std::istringstream ss(val); ss >> cur.rot.x   >> cur.rot.y   >> cur.rot.z;   }
        else if (key == "scale") { std::istringstream ss(val); ss >> cur.scale.x >> cur.scale.y >> cur.scale.z; }
    }
    flush(); // last object

    std::cout << "Scene loaded: " << filePath << " (" << objects.size() << " objects)\n";
    return true;
}

} // namespace SceneSerializer
