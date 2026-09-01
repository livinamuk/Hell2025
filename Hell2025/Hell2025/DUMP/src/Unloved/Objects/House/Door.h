#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"
#include "DeadLock.h"
#include "Unloved/Objects/Renderables/MeshBufferOLD.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Systems/House/ClippingVolume.h"
#include "Hell/ResourceManagement/Types/Model.h"

namespace Unloved {

struct Door {
    Door() = default;
    Door(uint64_t id, DoorCreateInfo& createInfo, SpawnOffset& spawnOffset);
    Door(const Door&) = delete;
    Door& operator=(const Door&) = delete;
    Door(Door&&) noexcept = default;
    Door& operator=(Door&&) noexcept = default;
    ~Door() = default;

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetRotationY(float value);
    void Update(float deltaTime);
	void CleanUp();
    void UpdateFloor();
    void SetEditorName(const std::string& name);
    void SetType(DoorType type);
    void SetFrontMaterial(DoorMaterialType type);
    void SetBackMaterial(DoorMaterialType type);
    void SetFrameFrontMaterial(DoorMaterialType type);
    void SetFrameBackMaterial(DoorMaterialType type);
    void SetDeadLockState(bool value);
    void SetSillState(bool value);
    void SetDeadLockedAtInitState(bool value);
    void SetOpenAtStartState(bool value);
    void SetMaxOpenValue(float value);
    void SetFloorPlaneMaterial(const std::string& materialName);
    void SetFloorPlaneTextureScale(float value);
    void SetFloorPlaneTextureOffsetU(float value);
    void SetFloorPlaneTextureOffsetV(float value);
    void SetFloorPlaneRotateTexture90(bool value);
    void SetFloorPlaneRoughnessFactor(float value);
    void SetFloorPlaneMetallicFactor(float value);
    void Reset();
    void DebugDraw();
    bool CameraFacingDoorWorldForward(const glm::vec3& cameraPositon, const glm::vec3& cameraForward);

	MeshNodes& GetMeshNodes()                                             { return m_meshNodes; }

    const bool IsDirty() const                                            { return m_meshNodes.IsDirty(); }
    const uint64_t GetObjectId() const                                    { return m_objectId; }
    const glm::vec3& GetPosition() const                                  { return m_position; }
    const glm::vec3& GetRotation() const                                  { return m_rotation; }
    const glm::vec3& GetInteractPosition() const                          { return m_interactPosition; }
    const glm::mat4& GetDoorModelMatrix () const                          { return m_doorModelMatrix; }
    const glm::mat4& GetDoorFrameModelMatrix () const                     { return m_frameModelMatrix; }
    const Model* GetDoorModel() const                                     { return m_doorModel; }
    const Model* GetDoorFrameModel() const                                { return m_frameModel; }
    const OpeningState& GetOpeningState() const                           { return m_openingState; }
    const DoorCreateInfo& GetCreateInfo() const                           { return m_createInfo; }
    const std::vector<RenderItem>& GetRenderItems() const                 { return m_renderItems; } // This includes main MeshNods render items plus any deadlocks render items ???
    const std::vector<RenderItem>& GetAdditionalStaticRenderItems() const { return m_additonalStaticRenderItems; }
    const std::string& GetEditorName() const                              { return m_createInfo.editorName; }
    const DoorType& GetType() const                                       { return m_createInfo.type; }
    const DoorMaterialType& GetMaterialTypeFront() const                  { return m_createInfo.materialTypeFront; }
    const DoorMaterialType& GetMaterialTypeBack() const                   { return m_createInfo.materialTypeBack; }
    const DoorMaterialType& GetMaterialTypeFrameFront() const             { return m_createInfo.materialTypeFrameFront; }
    const DoorMaterialType& GetMaterialTypeFrameBack() const              { return m_createInfo.materialTypeFrameBack; }
    const bool GetDeadLockState() const                                   { return m_createInfo.hasDeadLock; }
    const bool GetSillState() const                                       { return m_createInfo.hasSill; }
    const bool GetDeadLockedAtInitState() const                           { return m_createInfo.deadLockedAtInit; }
    const bool GetOpenAtStartState() const                                { return m_createInfo.openAtStart; }
    const AABB& GetPhsyicsAABB() const                                    { return m_physicsAABB; }
    const ClippingVolume& GetClippingVolume() const                       { return m_clippingVolume; }

    MeshBufferOLD m_raytracingDoorMesh;
    MeshBufferOLD m_raytracingFrameMesh;

private:
    void RecreateStaticAddionalRenderItems();

    void UpdateWorldForward();
    void CreateRaytracingVertices();
    void UpdateClippingVolume();

	DoorCreateInfo m_createInfo;
	MeshNodes m_meshNodes;
    SpawnOffset m_spawnOffset;
    ClippingVolume m_clippingVolume;

    AABB m_physicsAABB;

    std::vector<DeadLock> m_deadLocks;

    std::vector<RenderItem> m_renderItems; // This contains main MeshNods render items plus any deadlocks renderitems
    std::vector<RenderItem> m_additonalStaticRenderItems;

    //bool m_movedThisFrame = true;
    bool m_deadLocked = false;
    uint64_t m_lifeTime = 0;
    uint64_t m_objectId = 0;
    Model* m_doorModel = nullptr;
    Model * m_frameModel = nullptr;
    int32_t m_materialIndex = -1;
    OpeningState m_openingState = OpeningState::CLOSED;
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_interactPosition;
    float m_currentOpenRotation = 0;
    float m_maxOpenRotation = 1.8f;
    glm::mat4 m_doorModelMatrix = glm::mat4(1.0f);
    glm::mat4 m_frameModelMatrix = glm::mat4(1.0f);
    glm::vec3 m_localForward = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_worldForward = glm::vec3(1.0f, 0.0f, 0.0f);
};
}
