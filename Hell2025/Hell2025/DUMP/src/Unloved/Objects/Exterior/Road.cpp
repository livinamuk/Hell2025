#include "Road.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/Curve/Curve.h"
#include "Hell/Math/Math.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Projection/Projection.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <iostream> // TODO clean up logging
#include "Hell/Input.h"

namespace Unloved {
namespace Input = Hell::Input;


void Road::Init() {
    m_controlPoints2D.push_back(glm::vec2(26.4136, 11.1253));
    m_controlPoints2D.push_back(glm::vec2(31.6507, 7.2496));
    m_controlPoints2D.push_back(glm::vec2(37.1036, 7.7737));
    m_controlPoints2D.push_back(glm::vec2(41.9232, 4.17412));
    m_controlPoints2D.push_back(glm::vec2(48.2309, 4.86765));

    m_controlPoints3D.clear();

    for (glm::vec2& point : m_controlPoints2D) {
        glm::vec3 worldPosition = Hell::Physics::GetHeightMapPositionAtXZ(point.x, point.y);
        m_controlPoints3D.push_back(worldPosition);
    }

    RoadCurveType curveType = RoadCurveType::BEIZER;

    if (curveType == BEIZER) {
        float spacing = 1.0f;
        m_worldPoints = Hell::Curve::SampleBezierPath(m_controlPoints3D, spacing);

        // Snap to heightmap
        for (glm::vec3& point : m_worldPoints) {
            point = Hell::Physics::GetHeightMapPositionAtXZ(point.x, point.z);
        }
    }
}

void Road::Update() {
    //DrawPoints();
}

void Road::DrawPoints() {
    
    for (glm::vec3& point : m_worldPoints) {
        DebugDraw::DrawPoint(point, GREEN);
    }
    for (glm::vec3& point : m_controlPoints3D) {
        DebugDraw::DrawPoint(point, RED);
    }

    return;

    std::vector<ViewportData> viewportData = RenderDataManager::GetViewportData();
    if (viewportData.empty()) return;


    DebugDraw::DrawPoint(m_worldPoints[0], BLUE);

    glm::vec3 worldPos = m_worldPoints[0];
    glm::mat4 projectionView = viewportData[0].projectionView;
    int screenWidth = viewportData[0].width;
    int screenHeight = viewportData[0].height;
    bool flipY = false;


    glm::ivec2 mouseCoords = glm::ivec2(Input::GetMouseX(), Input::GetMouseY());
    glm::ivec2 pointCoords = Hell::Projection::WorldToScreen(worldPos, projectionView, screenWidth, screenHeight);


    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(0);
    glm::ivec2 mouseCoords2 = viewport->GetLocalMouseCoords();
    glm::ivec2 viewportCoords = viewport->WorldToScreen(viewportData[0].view, m_worldPoints[0]);

    bool hover = Hell::Math::WithinDistance(mouseCoords2, pointCoords, 10);
    if (hover) {
        DebugDraw::DrawPoint(m_worldPoints[0], YELLOW);
    }

    std::cout << "Point:          " << pointCoords.x << ", " << pointCoords.y << "\n";
    std::cout << "Mouse:          " << mouseCoords.x << ", " << mouseCoords.y << "\n";
    std::cout << "Mouse2:         " << mouseCoords2.x << ", " << mouseCoords2.y << "\n";
    std::cout << "viewportCoords: " << viewportCoords.x << ", " << viewportCoords.y << "\n";

}
}
