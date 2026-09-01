#pragma once

#include "RagdollAsset.h"

#include <string>

namespace RagdollMass {

    float ComputeShapeVolume(const RagdollShape& shape);
    float ComputeMarkerVolume(const RagdollMarkerAsset& marker);
    float ComputeTotalMass(const RagdollAsset& asset);
    bool Recalculate(RagdollAsset& asset, std::string& error);
}
