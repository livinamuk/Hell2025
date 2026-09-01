#include "Jetty.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <glm/common.hpp>

namespace Unloved {

    #define BOARD_SPACING 0.27f

    Jetty::Jetty(uint64_t id, JettyCreateInfo& createInfo, SpawnOffset& spawnOffset) {
        m_objectId = id;
        m_createInfo = createInfo;

        m_createInfo.position += spawnOffset.translation;
        m_createInfo.rotation.y += spawnOffset.yRotation;

        RecreateAll();
    }

    void Jetty::CleanUp() {
        m_renderItems.clear();
        Hell::Physics::MarkRigidStaticForRemoval(m_rigidStaticId);
    }

    void Jetty::Update() {

        //DebugDraw::DrawPoint(m_worldSpaceCenter, BLUE);

        //for (RenderItem& renderItem : m_renderItems) {
        //    glm::vec3 position = renderItem.modelMatrix[3];
        //    //DebugDraw::DrawPoint(position, YELLOW);
        //}
        //
        //
        //
        //RigidStatic* rigidStaticId = Hell::Physics::GetRigidStaitcById(m_rigidStaticId);
        //if (rigidStaticId) {
        //    DebugDraw::DrawAABB(rigidStaticId->GetAABB(), YELLOW);
        //    Logging::Debug() << rigidStaticId->GetAABB().GetBoundsMin() << ", " << rigidStaticId->GetAABB().GetBoundsMax() << "\n";
        //    return;
        //}
        //Logging::Debug() << "m_rigidStaticId: " << m_rigidStaticId << " is for some reaosn returning nullptr mn\n";
    }

    void Jetty::RecreateAll() {
        CleanUp();
        CreateBoardRenderItems();
        CreatePoleRenderItems();
        CreatePhysicsbjects();

        // Probably rethink the entire way you are doing this, coz it's a mess and confuses you every time you regrettably have to fuck around with it
        Unloved::WorldBVH::MarkStaticSceneBvhDirty();
    }

    void Jetty::SetPosition(const glm::vec3& position) {
        m_createInfo.position = position;
        RecreateAll();
    }

    void Jetty::SetRotation(const glm::vec3& rotation) {
        m_createInfo.rotation = rotation;
        RecreateAll();
    }

    void Jetty::SetScale(const glm::vec3& scale) {
        m_createInfo.scale = scale;
        RecreateAll();
    }

    void Jetty::SetBoardCount(uint32_t boardCount) {
        m_createInfo.boardCount = boardCount == 0 ? 1 : boardCount;
        RecreateAll();
    }

    void Jetty::CreateBoardRenderItems() {

        for (int i = 0; i < GetBoardCount(); i++) {
            std::string meshName = "";

            switch (i % 5) {
                case 0: meshName = "JettyBoard_0";  break;
                case 1: meshName = "JettyBoard_1";  break;
                case 2: meshName = "JettyBoard_2";  break;
                case 3: meshName = "JettyBoard_3";  break;
                case 4: meshName = "JettyBoard_4";  break;
                default: break;
            }

            int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Jetty");

            if (materialIndex == -1) {
                __debugbreak();
                Logging::Fatal() << "Yo, yer jetty material aint loading right m8\n";
            }

            // Model matrix
            Hell::Transform worldTransform;
            worldTransform.position = m_createInfo.position;
            worldTransform.rotation = m_createInfo.rotation;
            worldTransform.scale = m_createInfo.scale;

            Hell::Transform localTransform;
            localTransform.position.x = BOARD_SPACING * i;

            glm::mat4 modelMatrix = worldTransform.ToMat4() * localTransform.ToMat4();

            RenderItem renderItem = Unloved::RendererUtil::CreateAssetGeometryRenderItem("JettyPieces", meshName, modelMatrix, materialIndex, m_objectId);
            m_renderItems.push_back(renderItem);
        }


        // Ghetto hack. FIX!
        m_worldSpaceCenter = glm::vec3(0.0f);

        for (RenderItem& renderItem : m_renderItems) {
            m_worldSpaceCenter += (glm::vec3(renderItem.aabbMin) + glm::vec3(renderItem.aabbMax)) * 0.5f;
        }
        if (!m_renderItems.empty()) {
            m_worldSpaceCenter /= m_renderItems.size();
        }
    }

    void Jetty::CreatePoleRenderItems() {

        int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Jetty");
        if (materialIndex == -1) {
            __debugbreak();
            Logging::Fatal() << "Yo, yer jetty material aint loading right m8\n";
        }

        for (int i = 0; i < GetBoardCount(); i++) {

            if (i % 5 != 0) continue;

            // Model matrix
            Hell::Transform worldTransform;
            worldTransform.position = m_createInfo.position;
            worldTransform.rotation = m_createInfo.rotation;
            worldTransform.scale = m_createInfo.scale;

            Hell::Transform localTransform;
            localTransform.position.x = BOARD_SPACING * i;

            // Left pole
            localTransform.position.z = -1.0f;
            m_renderItems.push_back(Unloved::RendererUtil::CreateAssetGeometryRenderItem("JettyPieces", "Pole", worldTransform.ToMat4() * localTransform.ToMat4(), materialIndex, m_objectId));

            // Right pole
            localTransform.position.z = 1.0f;
            m_renderItems.push_back(Unloved::RendererUtil::CreateAssetGeometryRenderItem("JettyPieces", "Pole", worldTransform.ToMat4() * localTransform.ToMat4(), materialIndex, m_objectId));
        }
    }

    void Jetty::CreatePhysicsbjects() {

        Hell::Transform transform;
        transform.position = m_worldSpaceCenter;
        transform.rotation = m_createInfo.rotation;

        glm::vec3 boxExtents = glm::vec3(BOARD_SPACING * GetBoardCount(), 0.05f, 2.59f) * glm::abs(m_createInfo.scale);

        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);

        bool kinematic = false;

        m_rigidStaticId = Hell::Physics::CreateRigidStaticBoxFromExtents(transform, boxExtents, filterData, Transform());

        PhysicsUserData physicsUserData;
        physicsUserData.objectId = m_objectId;

        Hell::Physics::SetRigidStaticUserData(m_rigidStaticId, physicsUserData);
    }
}
