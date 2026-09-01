#include "PictureFrame.h"
#include "Hell/Logging.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

namespace Unloved {

PictureFrame::PictureFrame(uint64_t id, PictureFrameCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;

    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation += glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    SelectRandomPicture();
}

void PictureFrame::CleanUp() {
    WorldBVH::MarkStaticSceneBvhDirty();
    m_meshNodes.CleanUp();
}

void PictureFrame::Update() {
    Transform transform;
    transform.position = m_createInfo.position;
    transform.rotation = m_createInfo.rotation;
    transform.scale = m_createInfo.scale;

    m_meshNodes.Update(transform.to_mat4());
}

void PictureFrame::SelectRandomPicture() {
    std::string materialName = "CheckerBoard";

    if (m_createInfo.type == PictureFrameType::BIG_LANDSCAPE) {
        if (m_createInfo.useRandom) {
            materialName = HouseBuilder::GetNextRandomLargePictureFrameMaterial();
        }
        else {
            materialName = m_createInfo.materialName;
            HouseBuilder::TakeLargePictureFrameMaterial(materialName);
        }
    }
    else {
        // TODO
    }

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& picture = meshNodeCreateInfoSet.emplace_back();
    picture.meshName = "picture_low.003";
    picture.materialName = materialName;

    MeshNodeCreateInfo& frame = meshNodeCreateInfoSet.emplace_back();
    frame.meshName = "frame_side.L_low.022";
    frame.materialName = "PictureFrame0";

    m_meshNodes.Init(m_objectId, "PictureFrame_BigLandscape", meshNodeCreateInfoSet);
}

void PictureFrame::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
}

void PictureFrame::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
}

void PictureFrame::SetScale(const glm::vec3& scale) {
    m_createInfo.scale = scale;
}

void PictureFrame::SetType(PictureFrameType type) {
    if (m_createInfo.type == type) return;

    m_createInfo.type = type;
    m_meshNodes.CleanUp();
    SelectRandomPicture();
}

void PictureFrame::SetUseRandom(bool useRandom) {
    if (m_createInfo.useRandom == useRandom) return;

    m_createInfo.useRandom = useRandom;
    m_meshNodes.CleanUp();
    SelectRandomPicture();
}

void PictureFrame::SetMaterialName(const std::string& materialName) {
    if (m_createInfo.materialName == materialName) return;

    m_createInfo.materialName = materialName;
    if (m_createInfo.useRandom) return;

    m_meshNodes.CleanUp();
    SelectRandomPicture();
}
}
