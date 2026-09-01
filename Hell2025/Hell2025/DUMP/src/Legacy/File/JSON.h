#pragma once
#include "Hell/Math/VecXZ.h"
#include "Hell/Serialization/Json.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/Types.h"

#include <glm/glm.hpp>

#include <map>

namespace nlohmann {
    void to_json(nlohmann::json& j, const EditableAxisSpan& editableAxisSpan);
    void to_json(nlohmann::json& j, const Unloved::SequencePoint& sequencePoint);
    void to_json(nlohmann::json& j, const ChristmasLightsCreateInfo& info);
    void to_json(nlohmann::json& j, const DDGIVolumeCreateInfo& info);
    void to_json(nlohmann::json& j, const DobermannCreateInfo& info);
    void to_json(nlohmann::json& j, const DoorCreateInfo& info);
    void to_json(nlohmann::json& j, const FenceCreateInfo& info);
    void to_json(nlohmann::json& j, const FireplaceCreateInfo& info);
    void to_json(nlohmann::json& j, const GenericAnimatedObjectCreateInfo& info);
    void to_json(nlohmann::json& j, const GenericObjectCreateInfo& info);
    void to_json(nlohmann::json& j, const HouseLocationCreateInfo& createInfo);
    void to_json(nlohmann::json& j, const JettyCreateInfo& info);
    void to_json(nlohmann::json& j, const KangarooCreateInfo& info);
    void to_json(nlohmann::json& j, const LadderCreateInfo& info);
    void to_json(nlohmann::json& j, const LadderDismountCreateInfo& info);
    void to_json(nlohmann::json& j, const LightCreateInfo& info);
    void to_json(nlohmann::json& j, const MermaidCreateInfo& info);
    void to_json(nlohmann::json& j, const PianoCreateInfo& info);
    void to_json(nlohmann::json& j, const PickUpCreateInfo& info);
    void to_json(nlohmann::json& j, const PictureFrameCreateInfo& info);
    void to_json(nlohmann::json& j, const PowerPoleSetCreateInfo& info);
    void to_json(nlohmann::json& j, const PlanarQuadObjectCreateInfo& info);
    void to_json(nlohmann::json& j, const PointPairCreateInfo& info);
    void to_json(nlohmann::json& j, const SharkCreateInfo& info);
    void to_json(nlohmann::json& j, const SpawnPointCreateInfo& createInfo);
    void to_json(nlohmann::json& j, const StaircaseCreateInfo& info);
    void to_json(nlohmann::json& j, const TreeCreateInfo& info);
    void to_json(nlohmann::json& j, const WallCreateInfo& info);
    void to_json(nlohmann::json& j, const WindowCreateInfo& info);
    void to_json(nlohmann::json& j, const WorldPlaneCreateInfo& info);

    void from_json(const nlohmann::json& j, EditableAxisSpan& editableAxisSpan);
    void from_json(const nlohmann::json& j, Unloved::SequencePoint& sequencePoint);
    void from_json(const nlohmann::json& j, ChristmasLightsCreateInfo& info);
    void from_json(const nlohmann::json& j, DDGIVolumeCreateInfo& info);
    void from_json(const nlohmann::json& j, DobermannCreateInfo& info);
    void from_json(const nlohmann::json& j, DoorCreateInfo& info);
    void from_json(const nlohmann::json& j, FenceCreateInfo& info);
    void from_json(const nlohmann::json& j, FireplaceCreateInfo& info);
    void from_json(const nlohmann::json& j, GenericAnimatedObjectCreateInfo& info);
    void from_json(const nlohmann::json& j, GenericObjectCreateInfo& info);
    void from_json(const nlohmann::json& j, HouseLocationCreateInfo& createInfo);
    void from_json(const nlohmann::json& j, JettyCreateInfo& info);
    void from_json(const nlohmann::json& j, KangarooCreateInfo& info);
    void from_json(const nlohmann::json& j, LadderCreateInfo& info);
    void from_json(const nlohmann::json& j, LadderDismountCreateInfo& info);
    void from_json(const nlohmann::json& j, LightCreateInfo& info);
    void from_json(const nlohmann::json& j, MermaidCreateInfo& info);
    void from_json(const nlohmann::json& j, PianoCreateInfo& info);
    void from_json(const nlohmann::json& j, PickUpCreateInfo& info);
    void from_json(const nlohmann::json& j, PictureFrameCreateInfo& info);
    void from_json(const nlohmann::json& j, PowerPoleSetCreateInfo& info);
    void from_json(const nlohmann::json& j, PlanarQuadObjectCreateInfo& info);
    void from_json(const nlohmann::json& j, PointPairCreateInfo& info);
    void from_json(const nlohmann::json& j, SharkCreateInfo& info);
    void from_json(const nlohmann::json& j, SpawnPointCreateInfo& info);
    void from_json(const nlohmann::json& j, StaircaseCreateInfo& info);
    void from_json(const nlohmann::json& j, TreeCreateInfo& info);
    void from_json(const nlohmann::json& j, WallCreateInfo& info);
    void from_json(const nlohmann::json& j, WindowCreateInfo& info);
    void from_json(const nlohmann::json& j, WorldPlaneCreateInfo& info);

    void to_json(nlohmann::json& j, const std::map<Hell::ivecXZ, std::string>& map);
}

namespace JSON {
    bool LoadJsonFromFile(nlohmann::json& json, const std::string filepath);
    void SaveToFile(nlohmann::json& json, const std::string& filepath);

    AdditionalMapData AdditionalMapDataFromJSON(const std::string& jsonString);
    CreateInfoCollection CreateInfoCollectionFromJSONString(const std::string& jsonString);
    CreateInfoCollection CreateInfoCollectionFromJSONObject(nlohmann::json& json);
    std::string AdditionalMapDataToJSON(AdditionalMapData& additionalMapData);
    std::string CreateInfoCollectionToJSON(CreateInfoCollection& createInfoCollection);
}
