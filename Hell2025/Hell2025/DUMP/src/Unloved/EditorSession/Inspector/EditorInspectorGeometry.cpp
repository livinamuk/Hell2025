#include "EditorInspectorInternal.h"

#include "Unloved/EditorSession/UI/EditorDialogs.h"
#include "Unloved/EditorSession/UI/EditorInputElements.h"
#include "Unloved/EditorSession/Interaction/EditorPointSequences.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"

#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/World/World.h"

#include <string>
#include <vector>

namespace Unloved::EditorSession::Inspector::Internal {
    namespace {
        void SetGizmoToSelectedPoint(uint64_t objectId) {
            glm::vec3 position;
            if (PointSequences::GetPointPosition(objectId, Selection::GetSelectedPointIndex(), Selection::GetSelectedPointHandleType(), position)) {
                Gizmo::SetPosition(position);
            }
        }

        void SetEditorName(uint64_t objectId, const std::string& editorName) {
            if (!World::SetEditorNameById(objectId, editorName)) {
                Dialog::Open("Name '" + editorName + "' Taken");
            }
        }

        void SetObjectPosition(uint64_t objectId, const glm::vec3& position) {
            if (World::SetPositionById(objectId, position)) {
                SetGizmoToSelectedPoint(objectId);
            }
        }

        void SetObjectRotation(uint64_t objectId, const glm::vec3& rotation) {
            if (World::SetRotationById(objectId, rotation)) {
                SetGizmoToSelectedPoint(objectId);
            }
        }

        void SetSelectedPointPosition(uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType, const glm::vec3& position) {
            if (PointSequences::SetPointPosition(objectId, pointIndex, handleType, position)) {
                SetGizmoToSelectedPoint(objectId);
            }
        }

        void AddDeckingBoardsProperties(InputElements::PropertyList& properties, uint64_t objectId, std::string& materialName, bool& rotateUVs, float& uvScale) {
            static const std::vector<std::string> materials = {
                "NumGrid",
                "WeatherBoards_Bare",
                "DeckingBoards",
                "DeckingBoards2",
                "DeckingBoards3"
            };

            PlanarQuadObject* object = World::GetPlanarQuadObjectByObjectId(objectId);
            if (!object) return;

            properties.CheckBox("Rotate UVs", rotateUVs, [object, rotateUVs = &rotateUVs] { object->SetCustomBool(0, *rotateUVs); });
            properties.Float(objectId, "UV Scale", uvScale, [object, uvScale = &uvScale] { object->SetCustomFloat(0, *uvScale); });
            properties.DropDown(objectId, "Material", materials, materialName, [object, materialName = &materialName] { object->SetDeckingBoardsMaterial(*materialName); });
        }

        void AddDeckingPostProperties(InputElements::PropertyList& properties, uint64_t objectId, std::string& materialName, bool& rotateUVs, float& uvScale) {
            static const std::vector<std::string> materials = {
                "NumGrid",
                "WeatherBoards_Bare",
                "WoodOak"
            };

            PlanarQuadObject* object = World::GetPlanarQuadObjectByObjectId(objectId);
            if (!object) return;

            properties.CheckBox("Rotate UVs", rotateUVs, [object, rotateUVs = &rotateUVs] { object->SetCustomBool(0, *rotateUVs); });
            properties.Float(objectId, "UV Scale", uvScale, [object, uvScale = &uvScale] { object->SetCustomFloat(0, *uvScale); });
            properties.DropDown(objectId, "Material", materials, materialName, [object, materialName = &materialName] { object->SetDeckingBoardsMaterial(*materialName); });
        }

        void AddDeckingBearerProperties(InputElements::PropertyList& properties, uint64_t objectId, std::string& materialName, bool& rotateUVs, float& uvScale) {
            static const std::vector<std::string> materials = {
                "NumGrid",
                "WeatherBoards_Bare",
                "WoodOak"
            };

            PlanarQuadObject* object = World::GetPlanarQuadObjectByObjectId(objectId);
            if (!object) return;

            properties.CheckBox("Rotate UVs", rotateUVs, [object, rotateUVs = &rotateUVs] { object->SetCustomBool(0, *rotateUVs); });
            properties.Float(objectId, "UV Scale", uvScale, [object, uvScale = &uvScale] { object->SetCustomFloat(0, *uvScale); });
            properties.DropDown(objectId, "Material", materials, materialName, [object, materialName = &materialName] { object->SetDeckingBoardsMaterial(*materialName); });
        }
    }



    void RenderPlanarQuadProperties(const EditorRect& rect, uint64_t objectId) {
        PlanarQuadObject* object = World::GetPlanarQuadObjectByObjectId(objectId);
        InputElements::PropertyList properties;
        if (!object) {
            properties.Render(rect);
            return;
        }

        std::string editorName = object->GetEditorName();
        std::string materialName = object->GetCreateInfo().materialNames[0];
        glm::vec3 position = object->GetPosition();
        glm::vec3 rotation = object->GetRotation();
        bool rotateUVs = object->GetCreateInfo().customBools[0];
        float uvScale = object->GetCreateInfo().customFloats[0];

        properties.String(objectId, "Name", editorName, [objectId, editorName = &editorName] { SetEditorName(objectId, *editorName); });
        properties.Vec3(objectId, "Position", position, [objectId, position = &position] { SetObjectPosition(objectId, *position); });
        properties.Vec3(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetObjectRotation(objectId, *rotation); });

        if (Selection::HasSelectedPoint()) {
            glm::vec3 pointPosition;
            const int32_t pointIndex = Selection::GetSelectedPointIndex();
            const PointSequences::PointHandleType handleType = Selection::GetSelectedPointHandleType();

            if (PointSequences::GetPointPosition(objectId, pointIndex, handleType, pointPosition)) {
                properties.Vec3(objectId, "Point", pointPosition, [objectId, pointIndex, handleType, pointPosition = &pointPosition] { SetSelectedPointPosition(objectId, pointIndex, handleType, *pointPosition); });
            }

            properties.Render(rect);
            return;
        }

        switch (object->GetType()) {
            case PlanarQuadObjectType::DECKING_BOARDS: AddDeckingBoardsProperties(properties, objectId, materialName, rotateUVs, uvScale); break;
            default: break;
        }

        properties.Render(rect);
    }

    void RenderPointPairProperties(const EditorRect& rect, uint64_t objectId) {
        PointPairObject* object = World::GetPointPairObjectByObjectId(objectId);
        InputElements::PropertyList properties;
        if (!object) {
            properties.Render(rect);
            return;
        }

        std::string editorName = object->GetEditorName();
        std::string materialName = object->GetCreateInfo().materialNames[0];
        glm::vec3 position = object->GetPosition();
        glm::vec3 rotation = object->GetRotation();
        bool customBool0 = object->GetCreateInfo().customBools[0];
        float customFloat0 = object->GetCreateInfo().customFloats[0];

        properties.String(objectId, "Name", editorName, [objectId, editorName = &editorName] { SetEditorName(objectId, *editorName); });
        properties.Vec3(objectId, "Position", position, [objectId, position = &position] { SetObjectPosition(objectId, *position); });
        properties.Vec3(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetObjectRotation(objectId, *rotation); });

        if (Selection::HasSelectedPoint()) {
            glm::vec3 pointPosition;
            const int32_t pointIndex = Selection::GetSelectedPointIndex();
            const PointSequences::PointHandleType handleType = Selection::GetSelectedPointHandleType();

            if (PointSequences::GetPointPosition(objectId, pointIndex, handleType, pointPosition)) {
                properties.Vec3(objectId, "Point", pointPosition, [objectId, pointIndex, handleType, pointPosition = &pointPosition] { SetSelectedPointPosition(objectId, pointIndex, handleType, *pointPosition); });
            }
        }


        switch (object->GetType()) {
            case PointPairObjectType::DECKING_BEARER: AddDeckingBearerProperties(properties, objectId, materialName, customBool0, customFloat0); break;
            case PointPairObjectType::DECKING_POST:   AddDeckingPostProperties(properties, objectId, materialName, customBool0, customFloat0); break;
        }

        properties.Render(rect);
    }
}
