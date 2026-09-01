#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"

namespace Unloved {

struct HouseData {
    void SetFilename(const std::string& filename);
    void SetCreateInfoCollection(const CreateInfoCollection& createInfoCollection);

    CreateInfoCollection& GetCreateInfoCollection() { return m_createInfoCollection; }
    const CreateInfoCollection& GetCreateInfoCollection() const { return m_createInfoCollection; }
    const std::string& GetFilename() const { return m_filename; }

private:
    CreateInfoCollection m_createInfoCollection;
    std::string m_filename = UNDEFINED_STRING;
};
}
