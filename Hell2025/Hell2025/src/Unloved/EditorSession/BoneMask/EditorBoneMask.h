#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SkinnedModel;

namespace Unloved::EditorSession::BoneMaskEditor {

    void New(const std::string& name);
    bool Open(const std::string& path, std::string& error);
    bool Save(std::string& error);
    bool SetSkinnedModelName(const std::string& skinnedModelName);
    void Reset();
    void SelectBone(int32_t nodeIndex);
    void ClearSelection();
    void DrawSkeleton();

    bool HasDocument();
    bool HasSelectedBone();
    bool IsBoneSelected(int32_t nodeIndex);
    const std::string& GetName();
    const std::string& GetSourcePath();
    const std::string& GetSkinnedModelName();
    const std::string& GetSelectedBoneName();
    float GetSelectedBoneWeight();
    void SetSelectedBoneWeight(float weight);
    void ApplySelectedBoneWeightToChildren();
    const SkinnedModel* GetSkinnedModel();
    std::vector<std::string> GetAvailableSkinnedModelNames();
}
