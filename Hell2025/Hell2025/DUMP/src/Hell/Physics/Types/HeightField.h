#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/Math/VecXZ.h"

#include <cstdint>
#include <glm/vec3.hpp>

#pragma warning(push, 0)
#include <physx/PxShape.h>
#include <physx/PxRigidStatic.h>
#include <physx/geometry/PxHeightField.h>
#pragma warning(pop)

#include <vector>

using namespace physx;

struct HeightField {
    void Create(Hell::vecXZ& worldSpaceOffset, const float* heightValues, float heightScale, float rowScale, float colScale);
    void ActivatePhsyics();
    void DisablePhsyics();
    void MarkForRemoval();

    const bool HasActivePhysics() const     { return m_activePhysics; }
    const bool IsMarkedForRemoval() const   { return m_markedForRemoval; }
    Hell::vecXZ GetWorldSpaceOffset()       { return m_worldSpaceOffset; }
    PxHeightField* GetPxHeightField()       { return m_pxHeightField; }
    PxRigidStatic* GetPxRigidStatic()       { return m_pxRigidStatic; }
    PxShape* GetPxShape()                   { return m_pxShape; }
    const AABB& GetAABB()                   { return m_aabb; }

private:
    Hell::vecXZ m_worldSpaceOffset;
    PxHeightField* m_pxHeightField = nullptr;
    PxRigidStatic* m_pxRigidStatic = nullptr;
    PxShape* m_pxShape = nullptr;
    bool m_activePhysics = false;
    bool m_markedForRemoval = false;
    AABB m_aabb;
    uint64_t m_physicsId = 0;
};
