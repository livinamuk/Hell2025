#pragma once
#pragma warning(push, 0)
#include <physx/extensions/PxD6Joint.h>
#pragma warning(pop)

using namespace physx;

struct D6Joint {
    void SetPxD6Joint(PxD6Joint* pxD6Joint);
    void MarkForRemoval();

    PxD6Joint* GetPxD6Joint()   { return m_pxD6Joint; }
    bool IsMarkedForRemoval()   { return m_markedForRemoval; }

private:
    PxD6Joint* m_pxD6Joint = nullptr;
    bool m_markedForRemoval = false;
};
