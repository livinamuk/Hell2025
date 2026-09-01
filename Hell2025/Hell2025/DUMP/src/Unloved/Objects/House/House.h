#pragma once
#include "Unloved/Common/Types.h"

namespace Unloved {

struct House {
    void Init(uint64_t objectId);

    uint64_t GetBvhId()     { return m_bvhId; }
    uint64_t GetObjectId()  { return m_objectId; }

private:
    uint64_t m_bvhId = 0;
    uint64_t m_objectId = 0;
};
}
