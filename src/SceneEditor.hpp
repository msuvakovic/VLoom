#pragma once
#include <imgui.h>
#include <vector>
#include "SceneObject.hpp"

struct SceneEditor {
    void draw(std::vector<SceneObject>& objects) {
        for (auto& obj : objects) {
            ImGui::PushID(obj.label.c_str());
            if (ImGui::CollapsingHeader(obj.label.c_str())) {
                bool changed = false;

                ImGui::Indent();

                if (ImGui::TreeNode("Position")) {
                    changed |= ImGui::DragFloat("X##pos", &obj.pos.x, 0.1f);
                    changed |= ImGui::DragFloat("Y##pos", &obj.pos.y, 0.1f);
                    changed |= ImGui::DragFloat("Z##pos", &obj.pos.z, 0.1f);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Rotation")) {
                    changed |= ImGui::DragFloat("X##rot", &obj.rot.x, 1.0f);
                    changed |= ImGui::DragFloat("Y##rot", &obj.rot.y, 1.0f);
                    changed |= ImGui::DragFloat("Z##rot", &obj.rot.z, 1.0f);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Scale")) {
                    changed |= ImGui::DragFloat("X##scl", &obj.scale.x, 0.01f, 0.01f, 100.0f);
                    changed |= ImGui::DragFloat("Y##scl", &obj.scale.y, 0.01f, 0.01f, 100.0f);
                    changed |= ImGui::DragFloat("Z##scl", &obj.scale.z, 0.01f, 0.01f, 100.0f);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Texture")) {
                    ImGui::DragFloat("Tile##tex", &obj.texTile, 0.1f, 0.01f, 64.0f);
                    ImGui::TreePop();
                }

                ImGui::Unindent();

                if (changed) obj.rebuildTransform();
            }
            ImGui::PopID();
        }
    }
};
