#include "PowerPoleSet.h"
#include "Hell/Common/Bit.h"
#include "Hell/Curve/Curve.h"
#include "Hell/Logging.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Objects/Exterior/Wire.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <cmath>

namespace {
    glm::vec3 ApplySpawnOffset(const glm::vec3& point, const SpawnOffset& spawnOffset) {
        const float c = std::cos(spawnOffset.yRotation);
        const float s = std::sin(spawnOffset.yRotation);
        const glm::vec2 rotated(point.x * c + point.z * s, -point.x * s + point.z * c);
        return glm::vec3(rotated.x + spawnOffset.translation.x, point.y, rotated.y + spawnOffset.translation.z);
    }
}

namespace Unloved {

PowerPoleSet::PowerPoleSet(uint64_t id, PowerPoleSetCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    for (SequencePoint& sequencePoint : m_createInfo.sequencePoints) {
        sequencePoint.position = ApplySpawnOffset(sequencePoint.position, spawnOffset);
    }
    m_spawnOffset = SpawnOffset();

    Init();
}

void PowerPoleSet::AddControlPoint(const glm::vec2& controlPoint2D) {
    SequencePoint sequencePoint;
    sequencePoint.position = Hell::Physics::GetHeightMapPositionAtXZ(controlPoint2D.x, controlPoint2D.y);
    if (sequencePoint.position == glm::vec3(0.0f)) sequencePoint.position = glm::vec3(controlPoint2D.x, 0.0f, controlPoint2D.y);
    m_createInfo.sequencePoints.push_back(sequencePoint);

    Init();
}

void PowerPoleSet::SetPosition(const glm::vec3& position) {
    if (m_createInfo.sequencePoints.empty()) return;

    const glm::vec3 offset = position - m_createInfo.sequencePoints.front().position;
    for (SequencePoint& sequencePoint : m_createInfo.sequencePoints) sequencePoint.position += offset;
    Init();
}

void PowerPoleSet::UpdateSequencePoints(const std::vector<SequencePoint>& sequencePoints) {
    m_createInfo.sequencePoints = sequencePoints;
    Init();
}

void PowerPoleSet::Init() {
    CleanUp();
    m_position = m_createInfo.sequencePoints.empty() ? glm::vec3(0.0f) : m_createInfo.sequencePoints.front().position;

    std::vector<MeshNodeCreateInfo> emptyMeshNodeCreateInfoSet;

    m_meshNodes.Init(NO_ID, "PowerPole", emptyMeshNodeCreateInfoSet);
    m_meshNodes.SetMeshMaterials("PowerPole");
    m_meshNodes.Update(glm::mat4(1.0f));

    std::vector<glm::vec3> controlPoints3D;
    for (const SequencePoint& sequencePoint : m_createInfo.sequencePoints) controlPoints3D.push_back(sequencePoint.position);

    float spacing = 7.0f;
    m_finalPositions = Hell::Curve::SampleBezierPath(controlPoints3D, spacing);

    // Error check
    if (m_finalPositions.size() < 2) {
        Logging::Error() << "PowerPoleSet::Init() failed because there were less than 2 final positions";
        return;
    }

    std::vector<RenderItem> meshNodeRenderItems = m_meshNodes.GetRenderItems();

    glm::vec3 localWirePositionBackA = m_meshNodes.GetBoneLocalMatrix("A_back")[3];
    glm::vec3 localWirePositionBackB = m_meshNodes.GetBoneLocalMatrix("B_back")[3];
    glm::vec3 localWirePositionBackC = m_meshNodes.GetBoneLocalMatrix("C_back")[3];
    glm::vec3 localWirePositionBackD = m_meshNodes.GetBoneLocalMatrix("D_back")[3];
    glm::vec3 localWirePositionFrontA = m_meshNodes.GetBoneLocalMatrix("A_front")[3];
    glm::vec3 localWirePositionFrontB = m_meshNodes.GetBoneLocalMatrix("B_front")[3];
    glm::vec3 localWirePositionFrontC = m_meshNodes.GetBoneLocalMatrix("C_front")[3];
    glm::vec3 localWirePositionFrontD = m_meshNodes.GetBoneLocalMatrix("D_front")[3];

    for (int i = 0; i < m_finalPositions.size() - 1; i++) {
        glm::vec3 position = m_finalPositions[i] * glm::vec3(1.0f, 0.0f, 1.0f);
        glm::vec3 nextPosition = m_finalPositions[i + 1] * glm::vec3(1.0f, 0.0f, 1.0f);

        Transform transform;
        transform.position = m_finalPositions[i];
        transform.rotation.y = Hell::Math::YawBetweenPoints(position, nextPosition) + (HELL_PI * 0.5f);

        for (RenderItem& renderItem : meshNodeRenderItems) {
            renderItem.modelMatrix = transform.to_mat4();
            renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
            RendererUtil::UpdateRenderItemAABB(renderItem);
            m_renderItems.push_back(renderItem);
        }

        // Get wire
        m_wirePositionsBackA.push_back(transform.to_mat4() * glm::vec4(localWirePositionBackA, 1.0f));
        m_wirePositionsBackB.push_back(transform.to_mat4() * glm::vec4(localWirePositionBackB, 1.0f));
        m_wirePositionsBackC.push_back(transform.to_mat4() * glm::vec4(localWirePositionBackC, 1.0f));
        m_wirePositionsBackD.push_back(transform.to_mat4() * glm::vec4(localWirePositionBackD, 1.0f));
        m_wirePositionsFrontA.push_back(transform.to_mat4() * glm::vec4(localWirePositionFrontA, 1.0f));
        m_wirePositionsFrontB.push_back(transform.to_mat4() * glm::vec4(localWirePositionFrontB, 1.0f));
        m_wirePositionsFrontC.push_back(transform.to_mat4() * glm::vec4(localWirePositionFrontC, 1.0f));
        m_wirePositionsFrontD.push_back(transform.to_mat4() * glm::vec4(localWirePositionFrontD, 1.0f));
    }

    for (int i = 0; i < m_wirePositionsBackA.size() - 1; i++) {
        auto addWire = [&](const glm::vec3& begin, const glm::vec3& end) {
            WireCreateInfo wireCreateInfo;
            wireCreateInfo.sequencePoints.resize(2);
            wireCreateInfo.sequencePoints[0].position = begin;
            wireCreateInfo.sequencePoints[1].position = end;
            wireCreateInfo.sequencePoints[1].customFloat = 0.5f;
            wireCreateInfo.radius = 0.015f;
            wireCreateInfo.spacing = 2.0f;
            wireCreateInfo.parentObjectId = m_objectId;
            m_wireIds.push_back(World::AddWire(wireCreateInfo));
        };

        addWire(m_wirePositionsBackA[i], m_wirePositionsFrontA[i + 1]);
        addWire(m_wirePositionsBackB[i], m_wirePositionsFrontB[i + 1]);
        addWire(m_wirePositionsBackC[i], m_wirePositionsFrontC[i + 1]);
        addWire(m_wirePositionsBackD[i], m_wirePositionsFrontD[i + 1]);
    }

    for (RenderItem& renderItem : m_renderItems) {
        Hell::Bit::PackUint64(m_objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);
        RendererUtil::UpdateRenderItemAABB(renderItem);
    }
}

void PowerPoleSet::Update() {
    // Nothing as of yet
}

void PowerPoleSet::CleanUp() {
    for (uint64_t wireId : m_wireIds) {
        World::RemoveObjectById(wireId);
    }

    m_meshNodes.CleanUp();

    m_finalPositions.clear();
    m_wirePositionsBackA.clear();
    m_wirePositionsBackB.clear();
    m_wirePositionsBackC.clear();
    m_wirePositionsBackD.clear();
    m_wirePositionsFrontA.clear();
    m_wirePositionsFrontB.clear();
    m_wirePositionsFrontC.clear();
    m_wirePositionsFrontD.clear();
    m_renderItems.clear();
    m_wireIds.clear();
    WorldBVH::MarkStaticSceneBvhDirty();
}

const std::vector<RenderItem>& const PowerPoleSet::GetRenderItems() {
    return m_renderItems;
}
}
