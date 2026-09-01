#ifndef VULKAN_RAY_QUERY_TRIANGLE_GLSL
#define VULKAN_RAY_QUERY_TRIANGLE_GLSL

struct RayQueryTriangleHit {
    bool hitFound;
    float t;
    uint instanceCustomIndex;
    uint geometryIndex;
    uint primitiveIndex;
    bool frontFace;
    vec2 barycentrics;
    mat4x3 objectToWorld;
};

RayQueryTriangleHit EmptyRayQueryTriangleHit() {
    RayQueryTriangleHit hit;
    hit.hitFound = false;
    hit.t = 0.0;
    hit.instanceCustomIndex = 0u;
    hit.geometryIndex = 0u;
    hit.primitiveIndex = 0u;
    hit.frontFace = true;
    hit.barycentrics = vec2(0.0);
    hit.objectToWorld = mat4x3(0.0);
    return hit;
}

bool RayQueryTraceClosestTriangle(accelerationStructureEXT accelerationStructure, uint rayFlags, uint cullMask, vec3 rayOrigin, float rayTMin, vec3 rayDirection, float rayTMax, out RayQueryTriangleHit hit) {
    hit = EmptyRayQueryTriangleHit();

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, accelerationStructure, rayFlags, cullMask, rayOrigin, rayTMin, rayDirection, rayTMax);

    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType == gl_RayQueryCandidateIntersectionTriangleEXT) {
            rayQueryConfirmIntersectionEXT(rayQuery);
        }
    }

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
        return false;
    }

    hit.hitFound = true;
    hit.t = rayQueryGetIntersectionTEXT(rayQuery, true);
    hit.instanceCustomIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
    hit.geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
    hit.primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    hit.frontFace = rayQueryGetIntersectionFrontFaceEXT(rayQuery, true);
    hit.barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    hit.objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
    return true;
}

bool RayQueryAnyTriangleHit(accelerationStructureEXT accelerationStructure, uint rayFlags, uint cullMask, vec3 rayOrigin, float rayTMin, vec3 rayDirection, float rayTMax) {
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, accelerationStructure, rayFlags, cullMask, rayOrigin, rayTMin, rayDirection, rayTMax);

    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType == gl_RayQueryCandidateIntersectionTriangleEXT) {
            return true;
        }
    }

    return rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT;
}

#endif
