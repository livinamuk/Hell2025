#pragma once
#include "HouseData.h"

namespace Unloved {

void HouseData::SetCreateInfoCollection(const CreateInfoCollection& createInfoCollection) {
    m_createInfoCollection = createInfoCollection;
}

void HouseData::SetFilename(const std::string& filename) {
    m_filename = filename;
}
}
