#include "Mermaid.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Debug/Scratch.h"
#include "Unloved/Render/RendererEnums.h"

#include <cmath>

namespace {
    glm::vec3 ApplySpawnOffset(const glm::vec3& position, const SpawnOffset& spawnOffset) {
        const float c = std::cos(spawnOffset.yRotation);
        const float s = std::sin(spawnOffset.yRotation);
        const glm::vec3 rotated(position.x * c + position.z * s, position.y, -position.x * s + position.z * c);
        return rotated + spawnOffset.translation;
    }
}

namespace Unloved {

Mermaid::Mermaid(uint64_t id, MermaidCreateInfo createInfo, SpawnOffset spawnOffset) {
    Init(id, createInfo, spawnOffset);
}

void Mermaid::Init(uint64_t id, MermaidCreateInfo createInfo, SpawnOffset spawnOffset) {
    m_createInfo = createInfo;
    m_createInfo.position = ApplySpawnOffset(m_createInfo.position, spawnOffset);
    m_createInfo.rotation.y += spawnOffset.yRotation;
    m_createInfo.shopTeleportPosition = ApplySpawnOffset(m_createInfo.shopTeleportPosition, spawnOffset);
    m_createInfo.shopTeleportEuler.y += spawnOffset.yRotation;
    m_spawnOffset = SpawnOffset();
    m_objectId = id;

    m_transform.position = m_createInfo.position;
    m_transform.rotation = m_createInfo.rotation;

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& rock = meshNodeCreateInfoSet.emplace_back();
    rock.meshName = "Rock";
    //rock.rigidDynamic.createObject = true; 
    //rock.rigidDynamic.kinematic = true;
    //rock.rigidDynamic.offsetTransform = Transform();
    //rock.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
    //rock.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    //rock.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);   
    //rock.rigidDynamic.shapeType = PhysicsShapeType::CONVEX_MESH;
    //rock.rigidDynamic.convexMeshModelName = "CollisionMesh_MermaidRock";
    rock.materialName = "Rock";

    m_meshNodes.Init(m_objectId, "Mermaid", meshNodeCreateInfoSet);
    m_meshNodes.SetMaterialByMeshName("Arms", "MermaidArms");
    m_meshNodes.SetMaterialByMeshName("Body", "MermaidBody");
    m_meshNodes.SetMaterialByMeshName("BoobTube", "BoobTube");
    m_meshNodes.SetMaterialByMeshName("EyelashLower", "MermaidLashes");
    m_meshNodes.SetMaterialByMeshName("EyelashUpper", "MermaidLashes");
    m_meshNodes.SetMaterialByMeshName("EyeLeft", "MermaidEye");
    m_meshNodes.SetMaterialByMeshName("EyeRight", "MermaidEye");
    m_meshNodes.SetMaterialByMeshName("Face", "MermaidFace");
    m_meshNodes.SetMaterialByMeshName("HairInner", "MermaidHair");
    m_meshNodes.SetMaterialByMeshName("HairOutta", "MermaidHair");
    m_meshNodes.SetMaterialByMeshName("HairScalp", "MermaidScalp");
    m_meshNodes.SetMaterialByMeshName("Nails", "Nails");
    //m_meshNodes.SetMaterialByMeshName("Rock", "Rock");
    m_meshNodes.SetMaterialByMeshName("Tail", "MermaidTail");
    m_meshNodes.SetMaterialByMeshName("TailFin", "MermaidTail");

    m_meshNodes.SetBlendingModeByMeshName("EyelashLower", BlendingMode::BLENDED);
    m_meshNodes.SetBlendingModeByMeshName("EyelashUpper", BlendingMode::BLENDED);
    m_meshNodes.SetBlendingModeByMeshName("HairScalp", BlendingMode::BLENDED);
    m_meshNodes.SetBlendingModeByMeshName("HairOutta", BlendingMode::HAIR);
    m_meshNodes.SetBlendingModeByMeshName("HairInner", BlendingMode::HAIR);

    m_meshNodes.SetBlendingModeByMeshName("HairOutta", BlendingMode::ALPHA_DISCARD);
    m_meshNodes.SetBlendingModeByMeshName("HairInner", BlendingMode::ALPHA_DISCARD);
}

void Mermaid::Update(float deltaTime) {
    m_transform.rotation.y = m_createInfo.rotation.y + HELL_PI * 0.5f;
    //m_transform.position.x = 36.6f;
    //m_transform.position.z = 36.4f;

    const bool topVisible = Debug::Scratch::GetBool("Mermaid Top", true);
    if (m_topVisible != topVisible) {
        m_topVisible = topVisible;
        m_meshNodes.SetBlendingModeByMeshName("BoobTube", topVisible ? BlendingMode::DEFAULT : BlendingMode::DO_NOT_RENDER);
    }

    UpdateRenderItems();

    m_worldForward = m_transform.to_mat4() * glm::vec4(m_localForward, 0.0f);

    //DebugDraw();
}

void Mermaid::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_transform.position = position;
}

void Mermaid::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_transform.rotation = rotation;
}

void Mermaid::DebugDraw() {
    glm::vec3 p1 = m_transform.position;
    glm::vec3 p2 = m_transform.position + m_worldForward;
    DebugDraw::DrawPoint(p1, YELLOW);
    DebugDraw::DrawPoint(p2, YELLOW);
    DebugDraw::DrawLine(p1, p2, YELLOW);
}

void Mermaid::UpdateRenderItems() {
    m_meshNodes.Update(m_transform.to_mat4());
}

void Mermaid::CleanUp() {
    // Nothing as of yet
}

}
