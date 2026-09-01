#include "EditorBoneMask.h"

#include "Hell/AssetLoader/AssetLoader.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/BoneMask.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"

#include <algorithm>
#include <glm/mat4x4.hpp>
#include <utility>

namespace Unloved::EditorSession::BoneMaskEditor {
    namespace {
        BoneMask g_boneMask;
        std::string g_sourcePath;
        int32_t g_selectedBoneNodeIndex = -1;
        bool g_loaded = false;

        glm::vec4 GetBoneWeightColor(const std::string& boneName) {
            float weight = 0.0f;
            auto weightIt = g_boneMask.weights.find(boneName);
            if (weightIt != g_boneMask.weights.end()) weight = std::clamp(weightIt->second, 0.0f, 1.0f);

            if (weight <= 0.5f) return RED + (YELLOW - RED) * weight * 2.0f;
            return YELLOW + (GREEN - YELLOW) * (weight - 0.5f) * 2.0f;
        }
    }

    bool New(const std::string& name, const std::string& skinnedModelName, std::string& error) {
        if (name.empty()) {
            error = "Enter a bone mask name";
            return false;
        }

        SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelByName(skinnedModelName);
        if (!skinnedModel || skinnedModel->GetBoneCount() == 0) {
            error = "Select a skinned model for the new bone mask";
            return false;
        }

        BoneMask boneMask;
        boneMask.name = name;
        boneMask.skinnedModelName = skinnedModelName;
        const std::string sourcePath = "res/bone_masks/" + name + ".bonemask";
        if (!Hell::AssetLoader::SaveBoneMask(sourcePath, boneMask, error)) return false;

        g_boneMask = std::move(boneMask);
        g_sourcePath = sourcePath;
        g_selectedBoneNodeIndex = -1;
        g_loaded = true;
        error.clear();
        return true;
    }

    bool Open(const std::string& path, std::string& error) {
        BoneMask boneMask;
        if (!Hell::AssetLoader::LoadBoneMask(path, boneMask, error)) return false;

        SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelByName(boneMask.skinnedModelName);
        if (!skinnedModel || skinnedModel->GetBoneCount() == 0) {
            error = "Skinned model '" + boneMask.skinnedModelName + "' is not available";
            return false;
        }

        g_boneMask = std::move(boneMask);
        g_sourcePath = path;
        g_selectedBoneNodeIndex = -1;
        g_loaded = true;
        error.clear();
        return true;
    }

    bool Save(std::string& error) {
        if (!g_loaded) {
            error = "No bone mask is open";
            return false;
        }
        if (!Hell::AssetLoader::SaveBoneMask(g_sourcePath, g_boneMask, error)) return false;

        error.clear();
        return true;
    }

    bool SetSkinnedModelName(const std::string& skinnedModelName) {
        if (!g_loaded) return false;
        if (!skinnedModelName.empty()) {
            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelByName(skinnedModelName);
            if (!skinnedModel || skinnedModel->GetBoneCount() == 0) return false;
        }
        if (g_boneMask.skinnedModelName == skinnedModelName) return true;

        g_boneMask.skinnedModelName = skinnedModelName;
        g_selectedBoneNodeIndex = -1;
        return true;
    }

    void Reset() {
        g_boneMask = {};
        g_sourcePath.clear();
        g_selectedBoneNodeIndex = -1;
        g_loaded = false;
    }

    void SelectBone(int32_t nodeIndex) {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!skinnedModel || nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(skinnedModel->m_nodes.size())) {
            g_selectedBoneNodeIndex = -1;
            return;
        }

        const Node& node = skinnedModel->m_nodes[nodeIndex];
        if (skinnedModel->m_boneMapping.find(node.name) == skinnedModel->m_boneMapping.end()) {
            g_selectedBoneNodeIndex = -1;
            return;
        }
        g_selectedBoneNodeIndex = nodeIndex;
    }

    void ClearSelection() {
        g_selectedBoneNodeIndex = -1;
    }

    void DrawSkeleton() {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!skinnedModel) return;

        std::vector<glm::mat4> globalTransforms(skinnedModel->m_nodes.size(), glm::mat4(1.0f));

        // Build the complete bind pose so transforms between deform bones are preserved
        for (size_t nodeIndex = 0; nodeIndex < skinnedModel->m_nodes.size(); nodeIndex++) {
            const Node& node = skinnedModel->m_nodes[nodeIndex];
            if (node.parentIndex >= 0 && node.parentIndex < static_cast<int32_t>(nodeIndex)) globalTransforms[nodeIndex] = globalTransforms[node.parentIndex] * node.localBindTransform;
            else globalTransforms[nodeIndex] = node.localBindTransform;
        }

        for (int32_t nodeIndex : skinnedModel->m_boneNodeIndices) {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(globalTransforms.size())) continue;

            const Node& node = skinnedModel->m_nodes[nodeIndex];
            const glm::vec3 position = glm::vec3(globalTransforms[nodeIndex][3]);
            const glm::vec4 boneColor = GetBoneWeightColor(node.name);
            const glm::vec4 pointColor = nodeIndex == g_selectedBoneNodeIndex ? glm::vec4(0.2f, 1.0f, 1.0f, 1.0f) : boneColor;
            DebugDraw::DrawPoint(position, pointColor);

            // Skip non-deform nodes when finding the line parent
            int32_t parentIndex = node.parentIndex;
            for (size_t depth = 0; depth < skinnedModel->m_nodes.size() && parentIndex >= 0; depth++) {
                if (parentIndex >= static_cast<int32_t>(skinnedModel->m_nodes.size())) {
                    parentIndex = -1;
                    break;
                }
                if (skinnedModel->m_boneMapping.find(skinnedModel->m_nodes[parentIndex].name) != skinnedModel->m_boneMapping.end()) break;
                parentIndex = skinnedModel->m_nodes[parentIndex].parentIndex;
            }
            if (parentIndex >= 0) {
                const glm::vec4 parentColor = GetBoneWeightColor(skinnedModel->m_nodes[parentIndex].name);
                DebugDraw::DrawLine(position, glm::vec3(globalTransforms[parentIndex][3]), boneColor, parentColor);
            }
        }
    }

    bool HasDocument() {
        return g_loaded;
    }

    bool HasSelectedBone() {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        return skinnedModel && g_selectedBoneNodeIndex >= 0 && g_selectedBoneNodeIndex < static_cast<int32_t>(skinnedModel->m_nodes.size());
    }

    bool IsBoneSelected(int32_t nodeIndex) {
        return nodeIndex >= 0 && nodeIndex == g_selectedBoneNodeIndex;
    }

    const std::string& GetName() {
        return g_boneMask.name;
    }

    const std::string& GetSourcePath() {
        return g_sourcePath;
    }

    const std::string& GetSkinnedModelName() {
        return g_boneMask.skinnedModelName;
    }

    const std::string& GetSelectedBoneName() {
        static const std::string EMPTY_STRING;
        if (!HasSelectedBone()) return EMPTY_STRING;
        return GetSkinnedModel()->m_nodes[g_selectedBoneNodeIndex].name;
    }

    float GetSelectedBoneWeight() {
        if (!HasSelectedBone()) return 0.0f;
        auto weight = g_boneMask.weights.find(GetSelectedBoneName());
        return weight == g_boneMask.weights.end() ? 0.0f : weight->second;
    }

    void SetSelectedBoneWeight(float weight) {
        if (!HasSelectedBone()) return;
        g_boneMask.weights[GetSelectedBoneName()] = std::clamp(weight, 0.0f, 1.0f);
    }

    void ApplySelectedBoneWeightToChildren() {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!HasSelectedBone() || !skinnedModel) return;

        const float weight = GetSelectedBoneWeight();
        for (int32_t nodeIndex : skinnedModel->m_boneNodeIndices) {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(skinnedModel->m_nodes.size())) continue;

            int32_t parentIndex = skinnedModel->m_nodes[nodeIndex].parentIndex;
            for (size_t depth = 0; depth < skinnedModel->m_nodes.size() && parentIndex >= 0; depth++) {
                if (parentIndex >= static_cast<int32_t>(skinnedModel->m_nodes.size())) break;
                if (parentIndex == g_selectedBoneNodeIndex) {
                    g_boneMask.weights[skinnedModel->m_nodes[nodeIndex].name] = weight;
                    break;
                }
                parentIndex = skinnedModel->m_nodes[parentIndex].parentIndex;
            }
        }
    }

    const SkinnedModel* GetSkinnedModel() {
        if (!g_loaded || g_boneMask.skinnedModelName.empty()) return nullptr;
        return Hell::ResourceManager::GetSkinnedModelByName(g_boneMask.skinnedModelName);
    }

    std::vector<std::string> GetAvailableSkinnedModelNames() {
        std::vector<std::string> skinnedModelNames;

        for (auto& entry : Hell::ResourceManager::GetSkinnedModels()) {
            SkinnedModel& skinnedModel = entry.second;
            if (skinnedModel.GetName().empty() || skinnedModel.GetBoneCount() == 0) continue;
            skinnedModelNames.push_back(skinnedModel.GetName());
        }

        std::sort(skinnedModelNames.begin(), skinnedModelNames.end());
        return skinnedModelNames;
    }
}
