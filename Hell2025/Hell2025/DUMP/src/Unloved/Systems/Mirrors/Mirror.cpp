#pragma once
#include "Mirror.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/Logging.h"
#include "Hell/Projection/Projection.h"
#include "Hell/ResourceManagement/ResourceManager.h"

Mirror::Mirror(uint64_t id, uint64_t parentId, uint32_t meshNodeIndex, uint32_t globalMeshIndex) {
    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(globalMeshIndex);
    m_objectId = id;
    m_parentId = parentId;
    m_meshNodeIndex = meshNodeIndex;
    m_globalMeshIndex = globalMeshIndex;

    // Validity checks
    if (!mesh) {
        Logging::Error() << "Mirror::Mirror(..) failed because globalMeshIndex '" << globalMeshIndex << "' returned nullptr";
        return;
    }

    Hell::MeshBuffer& assetGeometry = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    if (mesh->indexCount == 0) {
        Logging::Error() << "Mirror::Mirror(..) failed because mesh has no vertices";
        return;
    }

    // Store AABB
    m_localAabb = AABB(mesh->aabbMin, mesh->aabbMax);

    // Compute local corners
    glm::vec3 boundsMin = mesh->aabbMin;
    glm::vec3 boundsMax = mesh->aabbMax;
    glm::vec3 size = boundsMax - boundsMin;

    int thinAxis = 0;
    if (size.y < size.x && size.y < size.z) thinAxis = 1;
    else if (size.z < size.x && size.z < size.y) thinAxis = 2;

    m_localCorners.clear();
    m_worldCorners.clear();

    if (thinAxis == 0) {
        float x = boundsMin.x;
        m_localCorners.push_back({ x, boundsMin.y, boundsMin.z });
        m_localCorners.push_back({ x, boundsMax.y, boundsMin.z });
        m_localCorners.push_back({ x, boundsMin.y, boundsMax.z });
        m_localCorners.push_back({ x, boundsMax.y, boundsMax.z });
    }
    else if (thinAxis == 1) {
        float y = boundsMin.y;
        m_localCorners.push_back({ boundsMin.x, y, boundsMin.z });
        m_localCorners.push_back({ boundsMax.x, y, boundsMin.z });
        m_localCorners.push_back({ boundsMin.x, y, boundsMax.z });
        m_localCorners.push_back({ boundsMax.x, y, boundsMax.z });
    }
    else {
        float z = boundsMin.z;
        m_localCorners.push_back({ boundsMin.x, boundsMin.y, z });
        m_localCorners.push_back({ boundsMax.x, boundsMin.y, z });
        m_localCorners.push_back({ boundsMin.x, boundsMax.y, z });
        m_localCorners.push_back({ boundsMax.x, boundsMax.y, z });
    }

    // Compute local center
    m_localCenter = (boundsMin + boundsMax) * glm::vec3(0.5f);

    // Local normal
    const uint32_t firstIndex = assetGeometry.GetIndices()[mesh->baseIndex];
    const Vertex& firstVertex = assetGeometry.GetVertices()[firstIndex + mesh->baseVertex];
    m_localNormal = glm::normalize(firstVertex.normal);
}


void Mirror::Update(const glm::mat4& worldMatrix) {
    m_worldCenter = glm::vec3(worldMatrix * glm::vec4(m_localCenter, 1.0f)); 
    m_worldNormal = glm::normalize(glm::vec3(worldMatrix * glm::vec4(m_localNormal, 0.0f)));
    m_worldMatrix = worldMatrix;

    m_worldCorners.clear();

    for (const glm::vec3& localCorner : m_localCorners) {
        m_worldCorners.emplace_back(glm::vec3(worldMatrix * glm::vec4(localCorner, 1.0f)));
    }

    for (int i = 0; i < 4; i++) {
        m_clipPlanes[i] = glm::vec4(0.0f);
        m_reflectVectors[i] = glm::vec3(0.0f);
        m_viewMatrices[i] = glm::mat4(1.0f);
        m_facingViewportCamera[i] = false;

        if (Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i)) {
            if (!viewport->IsVisible()) continue;

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;
            
            const glm::vec3& cameraPosition = player->GetCameraPosition();
            const glm::vec3& cameraUp = player->GetCameraUp();
            const glm::vec3& cameraForward = player->GetCameraForward();;
            
            constexpr float kMirrorPlaneEpsilon = 1e-4f;
            glm::vec4 planeWorld(m_worldNormal, -glm::dot(m_worldNormal, m_worldCenter) + kMirrorPlaneEpsilon);

            float signedDistanceFromCamera = glm::dot(m_worldNormal, cameraPosition) + planeWorld.w;
            glm::vec3 mirrorCameraPos = cameraPosition - 2.0f * signedDistanceFromCamera * m_worldNormal;
            glm::vec3 R = cameraForward - 2.0f * glm::dot(m_worldNormal, cameraForward) * m_worldNormal;
            
            // Guard against camera almost perpendicular to mirror
            float len2 = glm::dot(R, R);
            if (len2 < 1e-6f) {
                R = cameraForward;
            }
            else {
                R = glm::normalize(R);
            }

            glm::vec3 reflectedUp = cameraUp - 2.0f * glm::dot(m_worldNormal, cameraUp) * m_worldNormal;
            glm::vec3 right = glm::normalize(glm::cross(R, reflectedUp));

            reflectedUp = glm::normalize(glm::cross(right, R));

            m_clipPlanes[i] = planeWorld;
            m_reflectVectors[i] = R;
            m_viewMatrices[i] = glm::lookAt(mirrorCameraPos, mirrorCameraPos + R, reflectedUp);

            // Find min/max bounds on the view-space plane by projecting corners onto the near plane
            // z is negative in view space, so negate to get a positive distance
            float leftEdge = 1e6f, rightEdge = -1e6f, bottomEdge = 1e6f, topEdge = -1e6f;
            float nearPlane = viewport->GetNearPlane();

            glm::vec3 viewCorners[4];
            for (int j = 0; j < 4; j++) {
                viewCorners[j] = glm::vec3(m_viewMatrices[i] * glm::vec4(m_worldCorners[j], 1.0f));
            }

            for (int j = 0; j < 4; j++) {
                float scale = nearPlane / -viewCorners[j].z;
                leftEdge = glm::min(leftEdge, viewCorners[j].x * scale);
                rightEdge = glm::max(rightEdge, viewCorners[j].x * scale);
                bottomEdge = glm::min(bottomEdge, viewCorners[j].y * scale);
                topEdge = glm::max(topEdge, viewCorners[j].y * scale);
            }

            // Create the tight asymmetric projection
            m_projectionMatrices[i] = glm::frustum(leftEdge, rightEdge, bottomEdge, topEdge, nearPlane, m_farDistance);

            // Now apply oblique clipping so objects behind the mirror surface are culled
            m_projectionMatrices[i] = Hell::Projection::Oblique(m_projectionMatrices[i], m_viewMatrices[i], planeWorld);

            // Update the frustum for culling
            m_frustums[i].Update(m_projectionMatrices[i] * m_viewMatrices[i]);

            // Is this mirror facing away from this viewport's camera?
            glm::vec3 toCamera = cameraPosition - m_worldCenter;
            glm::vec3 toMirror = m_worldCenter - cameraPosition;
            bool cameraOnFrontSide = glm::dot(m_worldNormal, toCamera) > 0.0f;      // Is camera in front of the mirror plane?
            bool mirrorInFrontOfCamera = glm::dot(cameraForward, toMirror) > 0.0f;  // Is the mirror in front of the camera in camera-forward space?
            m_facingViewportCamera[i] = cameraOnFrontSide && mirrorInFrontOfCamera;

            //DebugDraw();
        }
    }
}

void Mirror::DebugDraw() {
    if (m_worldCorners.size() != 4) {
        Logging::Error() << "Mirror::DebugDraw() failed: mirror doesn't have four corners";
        return;
    }

    for (const glm::vec3& point : m_worldCorners) {
        DebugDraw::DrawPoint(point, OUTLINE_COLOR);
    }

    glm::vec4 color = IsFacingViewportCamera(0) ? WHITE : RED;
    DebugDraw::DrawLine(m_worldCorners[0], m_worldCorners[1], color); // Left
    DebugDraw::DrawLine(m_worldCorners[2], m_worldCorners[3], color); // Right
    DebugDraw::DrawLine(m_worldCorners[1], m_worldCorners[3], color); // Top
    DebugDraw::DrawLine(m_worldCorners[0], m_worldCorners[2], color); // Bottom

    // World normal
    glm::vec3 p1 = m_worldCenter + (m_worldNormal * 0.1f);
    DebugDraw::DrawPoint(m_worldCenter, OUTLINE_COLOR);
    DebugDraw::DrawPoint(p1, OUTLINE_COLOR);
    DebugDraw::DrawLine(m_worldCenter, p1, WHITE);
}

bool Mirror::IsFacingViewportCamera(int viewportIndex) const {
    if (viewportIndex < 0 || viewportIndex >= 4) return false;
    return m_facingViewportCamera[viewportIndex];

}
const glm::vec4& Mirror::GetClipPlane(int viewportIndex) const {
    static const glm::vec4 out_of_range = glm::vec4(0.0f);
    if (viewportIndex < 0 || viewportIndex >= 4) return out_of_range;
    return m_clipPlanes[viewportIndex];

}

const glm::vec3& Mirror::GetReflectVector(int viewportIndex) const {
    static const glm::vec3 out_of_range = glm::vec3(0.0f);
    if (viewportIndex < 0 || viewportIndex >= 4) return out_of_range;
    return m_reflectVectors[viewportIndex];
}

const glm::mat4& Mirror::GetProjecionMatrix(int viewportIndex) const {
    static const glm::mat4 out_of_range(1.0f);
    if (viewportIndex < 0 || viewportIndex >= 4) return out_of_range;
    return m_projectionMatrices[viewportIndex];
}

const glm::mat4& Mirror::GetViewMatrix(int viewportIndex) const {
    static const glm::mat4 out_of_range(1.0f);
    if (viewportIndex < 0 || viewportIndex >= 4) return out_of_range;
    return m_viewMatrices[viewportIndex];
}

Unloved::Frustum* Mirror::GetFrustum(int viewportIndex) {
    if (viewportIndex < 0 || viewportIndex >= 4) return nullptr;
    return &m_frustums[viewportIndex];
}
