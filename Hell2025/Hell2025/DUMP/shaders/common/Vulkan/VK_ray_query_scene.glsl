#ifndef VULKAN_RAY_QUERY_SCENE_GLSL
#define VULKAN_RAY_QUERY_SCENE_GLSL

#include "VK_buffer_references.glsl"

const float RAY_QUERY_ALPHA_TEST_THRESHOLD = 0.25;
const float RAY_QUERY_VECTOR_EPSILON = 1.0e-8;

struct RayQueryPackedVertex {
    float vx, vy, vz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

struct RayQueryBLASData {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint sceneRenderItemIndexOffset;
    uint sceneRenderItemIndexCount;
    uint padding0;
    uint padding1;
};

layout(buffer_reference, scalar) readonly buffer RayQueryBLASDataBuffer {
    RayQueryBLASData blasData[];
};

layout(buffer_reference, scalar) readonly buffer RayQuerySceneRenderItemIndexBuffer {
    uint sceneRenderItemIndices[];
};

layout(buffer_reference, scalar) readonly buffer RayQueryVertexBuffer {
    RayQueryPackedVertex vertices[];
};

layout(buffer_reference, scalar) readonly buffer RayQueryIndexBuffer {
    uint indices[];
};

struct RayQueryContext {
    RayQueryBLASDataBuffer blasDataBuffer;
    RayQuerySceneRenderItemIndexBuffer sceneRenderItemIndexBuffer;
    RenderItemBuffer renderItemBuffer;
    MaterialBuffer materialBuffer;
};

struct RayQueryHit {
    bool found;
    bool frontFace;
    vec3 hitPos;
    vec3 hitNormal;
    vec3 hitTangent;
    vec3 rayDir;
    float rayT;
    int materialIndex;
    uint sceneRenderItemIndex;
    vec2 uv;
};

struct RayQueryMaterialSample {
    vec3 normal;
    vec3 linearBaseColor;
    float roughness;
    float metallic;
};

RayQueryContext CreateRayQueryContext(uint64_t blasDataDeviceAddress, uint64_t sceneRenderItemIndicesDeviceAddress, uint64_t renderItemBufferDeviceAddress, uint64_t materialBufferDeviceAddress) {
    RayQueryContext context;
    context.blasDataBuffer = RayQueryBLASDataBuffer(blasDataDeviceAddress);
    context.sceneRenderItemIndexBuffer = RayQuerySceneRenderItemIndexBuffer(sceneRenderItemIndicesDeviceAddress);
    context.renderItemBuffer = RenderItemBuffer(renderItemBufferDeviceAddress);
    context.materialBuffer = MaterialBuffer(materialBufferDeviceAddress);
    return context;
}

RayQueryHit EmptyRayQueryHit(vec3 rayDir) {
    RayQueryHit hit;
    hit.found = false;
    hit.frontFace = true;
    hit.hitPos = vec3(0.0);
    hit.hitNormal = vec3(0.0, 1.0, 0.0);
    hit.hitTangent = vec3(0.0);
    hit.rayDir = rayDir;
    hit.rayT = 0.0;
    hit.materialIndex = -1;
    hit.sceneRenderItemIndex = 0u;
    hit.uv = vec2(0.0);
    return hit;
}

bool RayQueryBlendingModeUsesAlphaMask(uint blendingMode) {
    return blendingMode == BLENDING_MODE_ALPHA_DISCARD || blendingMode == BLENDING_MODE_HAIR || blendingMode == BLENDING_MODE_HAIR_UNDER_LAYER;
}

bool ResolveRayQueryRenderItem(RayQueryContext context, RayQueryBLASData blasData, uint geometryIndex, out uint sceneRenderItemIndex, out RenderItem renderItem) {
    sceneRenderItemIndex = 0u;

    if (geometryIndex >= blasData.sceneRenderItemIndexCount) {
        return false;
    }

    sceneRenderItemIndex = context.sceneRenderItemIndexBuffer.sceneRenderItemIndices[blasData.sceneRenderItemIndexOffset + geometryIndex];
    renderItem = context.renderItemBuffer.renderItems[sceneRenderItemIndex];
    return true;
}

bool FetchRayQueryTriangle(RayQueryBLASData blasData, RenderItem renderItem, uint primitiveIndex, out RayQueryPackedVertex v0, out RayQueryPackedVertex v1, out RayQueryPackedVertex v2) {
    uint localIndexOffset = primitiveIndex * 3u;
    if (localIndexOffset + 2u >= renderItem.indexCount) {
        return false;
    }

    RayQueryIndexBuffer indexBuffer = RayQueryIndexBuffer(blasData.indexBufferDeviceAddress);
    uint i0 = indexBuffer.indices[renderItem.baseIndex + localIndexOffset + 0u] + renderItem.baseVertex;
    uint i1 = indexBuffer.indices[renderItem.baseIndex + localIndexOffset + 1u] + renderItem.baseVertex;
    uint i2 = indexBuffer.indices[renderItem.baseIndex + localIndexOffset + 2u] + renderItem.baseVertex;
    uint vertexEnd = renderItem.baseVertex + renderItem.vertexCount;

    if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd) {
        return false;
    }

    RayQueryVertexBuffer vertexBuffer = RayQueryVertexBuffer(blasData.vertexBufferDeviceAddress);
    v0 = vertexBuffer.vertices[i0];
    v1 = vertexBuffer.vertices[i1];
    v2 = vertexBuffer.vertices[i2];
    return true;
}

vec3 GetRayQueryBarycentricWeights(vec2 barycentrics) {
    return vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
}

vec2 InterpolateRayQueryUV(RayQueryPackedVertex v0, RayQueryPackedVertex v1, RayQueryPackedVertex v2, vec3 weights) {
    return vec2(v0.u, v0.v) * weights.x + vec2(v1.u, v1.v) * weights.y + vec2(v2.u, v2.v) * weights.z;
}

bool RayQueryCandidatePassesAlphaTest(RayQueryContext context, RayQueryBLASData blasData, RenderItem renderItem, uint primitiveIndex, vec2 barycentrics) {
    if (renderItem.materialIndex < 0) {
        return false;
    }

    RayQueryPackedVertex v0;
    RayQueryPackedVertex v1;
    RayQueryPackedVertex v2;
    if (!FetchRayQueryTriangle(blasData, renderItem, primitiveIndex, v0, v1, v2)) {
        return true;
    }

    Material material = context.materialBuffer.materials[renderItem.materialIndex];
    if (material.basecolor < 0) {
        return true;
    }

    vec3 weights = GetRayQueryBarycentricWeights(barycentrics);
    vec2 uv = InterpolateRayQueryUV(v0, v1, v2, weights);
    uint textureIndex = uint(material.basecolor);
    float alpha = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0).a;
    return alpha >= RAY_QUERY_ALPHA_TEST_THRESHOLD;
}

RayQueryHit TraceClosestSceneHitInternal(accelerationStructureEXT accelerationStructure, RayQueryContext context, vec3 rayOrigin, vec3 rayDir, float minDistance, float maxDistance, bool skipMirrors) {
    RayQueryHit result = EmptyRayQueryHit(rayDir);

    if (maxDistance <= minDistance) {
        return result;
    }

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, accelerationStructure, gl_RayFlagsNoneEXT, 0xff, rayOrigin, minDistance, rayDir, maxDistance);

    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASData blasData = context.blasDataBuffer.blasData[instanceIndex];
        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        uint sceneRenderItemIndex;
        RenderItem renderItem;

        if (!ResolveRayQueryRenderItem(context, blasData, geometryIndex, sceneRenderItemIndex, renderItem)) {
            continue;
        }

        if ((skipMirrors && renderItem.blendingMode == BLENDING_MODE_MIRROR) || renderItem.materialIndex < 0) {
            continue;
        }

        if (RayQueryBlendingModeUsesAlphaMask(renderItem.blendingMode)) {
            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);
            if (!RayQueryCandidatePassesAlphaTest(context, blasData, renderItem, primitiveIndex, barycentrics)) {
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
    }

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
        return result;
    }

    uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
    RayQueryBLASData blasData = context.blasDataBuffer.blasData[instanceIndex];
    uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
    uint sceneRenderItemIndex;
    RenderItem renderItem;

    if (!ResolveRayQueryRenderItem(context, blasData, geometryIndex, sceneRenderItemIndex, renderItem) || renderItem.materialIndex < 0) {
        return result;
    }

    uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    RayQueryPackedVertex v0;
    RayQueryPackedVertex v1;
    RayQueryPackedVertex v2;

    if (!FetchRayQueryTriangle(blasData, renderItem, primitiveIndex, v0, v1, v2)) {
        return result;
    }

    vec3 weights = GetRayQueryBarycentricWeights(barycentrics);
    vec3 objectNormal = vec3(v0.nx, v0.ny, v0.nz) * weights.x + vec3(v1.nx, v1.ny, v1.nz) * weights.y + vec3(v2.nx, v2.ny, v2.nz) * weights.z;
    vec3 objectTangent = vec3(v0.tx, v0.ty, v0.tz) * weights.x + vec3(v1.tx, v1.ty, v1.tz) * weights.y + vec3(v2.tx, v2.ty, v2.tz) * weights.z;
    mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
    mat4x3 worldToObject = rayQueryGetIntersectionWorldToObjectEXT(rayQuery, true);
    vec3 worldNormal = transpose(mat3(worldToObject)) * objectNormal;
    vec3 worldTangent = objectToWorld * vec4(objectTangent, 0.0);

    if (dot(worldNormal, worldNormal) <= RAY_QUERY_VECTOR_EPSILON) {
        return result;
    }

    worldNormal = normalize(worldNormal);
    if (dot(worldNormal, rayDir) > 0.0) {
        worldNormal = -worldNormal;
    }

    worldTangent -= dot(worldTangent, worldNormal) * worldNormal;
    if (dot(worldTangent, worldTangent) > RAY_QUERY_VECTOR_EPSILON) {
        worldTangent = normalize(worldTangent);
    } else {
        worldTangent = vec3(0.0);
    }

    float rayT = rayQueryGetIntersectionTEXT(rayQuery, true);
    result.found = true;
    result.frontFace = rayQueryGetIntersectionFrontFaceEXT(rayQuery, true);
    result.hitPos = rayOrigin + rayDir * rayT;
    result.hitNormal = worldNormal;
    result.hitTangent = worldTangent;
    result.rayT = rayT;
    result.materialIndex = renderItem.materialIndex;
    result.sceneRenderItemIndex = sceneRenderItemIndex;
    result.uv = InterpolateRayQueryUV(v0, v1, v2, weights);
    return result;
}

RayQueryHit TraceClosestSceneHit(accelerationStructureEXT accelerationStructure, RayQueryContext context, vec3 rayOrigin, vec3 rayDir, float minDistance, float maxDistance) {
    return TraceClosestSceneHitInternal(accelerationStructure, context, rayOrigin, rayDir, minDistance, maxDistance, false);
}

RayQueryHit TraceClosestReflectionHit(accelerationStructureEXT accelerationStructure, RayQueryContext context, vec3 rayOrigin, vec3 rayDir, float minDistance, float maxDistance) {
    return TraceClosestSceneHitInternal(accelerationStructure, context, rayOrigin, rayDir, minDistance, maxDistance, true);
}

RayQueryMaterialSample EvaluateRayHitMaterial(RayQueryContext context, RayQueryHit hit, float materialLod, float normalLod) {
    RayQueryMaterialSample materialSample;
    materialSample.normal = hit.hitNormal;
    materialSample.linearBaseColor = vec3(0.0);
    materialSample.roughness = 1.0;
    materialSample.metallic = 0.0;

    if (!hit.found || hit.materialIndex < 0) {
        return materialSample;
    }

    Material material = context.materialBuffer.materials[hit.materialIndex];

    if (material.basecolor >= 0) {
        uint textureIndex = uint(material.basecolor);
        vec3 baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), hit.uv, materialLod).rgb;
        materialSample.linearBaseColor = pow(baseColor, vec3(2.2));
    }

    if (material.rma >= 0) {
        uint textureIndex = uint(material.rma);
        vec4 rma = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), hit.uv, materialLod);
        materialSample.roughness = clamp(rma.r, 0.0, 1.0);
        materialSample.metallic = rma.g;
    }

    if (material.normal >= 0 && dot(hit.hitTangent, hit.hitTangent) > RAY_QUERY_VECTOR_EPSILON) {
        uint textureIndex = uint(material.normal);
        vec3 normalMap = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), hit.uv, normalLod).rgb * 2.0 - 1.0;
        vec3 bitangent = normalize(cross(hit.hitNormal, hit.hitTangent));
        vec3 mappedNormal = mat3(hit.hitTangent, bitangent, hit.hitNormal) * normalMap;

        if (dot(mappedNormal, mappedNormal) > RAY_QUERY_VECTOR_EPSILON) {
            materialSample.normal = normalize(mappedNormal);
            if (dot(materialSample.normal, hit.rayDir) > 0.0) {
                materialSample.normal = -materialSample.normal;
            }
        }
    }

    return materialSample;
}

bool TraceAnySceneHit(accelerationStructureEXT accelerationStructure, RayQueryContext context, vec3 rayOrigin, vec3 rayDir, float minDistance, float maxDistance) {
    if (maxDistance <= minDistance) {
        return false;
    }

    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, accelerationStructure, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, rayOrigin, minDistance, rayDir, maxDistance);

    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASData blasData = context.blasDataBuffer.blasData[instanceIndex];
        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        uint sceneRenderItemIndex;
        RenderItem renderItem;

        if (!ResolveRayQueryRenderItem(context, blasData, geometryIndex, sceneRenderItemIndex, renderItem)) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return true;
        }

        if (renderItem.materialIndex < 0) {
            continue;
        }

        if (RayQueryBlendingModeUsesAlphaMask(renderItem.blendingMode)) {
            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);
            if (!RayQueryCandidatePassesAlphaTest(context, blasData, renderItem, primitiveIndex, barycentrics)) {
                continue;
            }
        }

        rayQueryConfirmIntersectionEXT(rayQuery);
        return true;
    }

    return rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT;
}

float TraceSceneLineOfSight(accelerationStructureEXT accelerationStructure, RayQueryContext context, vec3 rayOrigin, vec3 target) {
    vec3 rayVector = target - rayOrigin;
    float rayLength = length(rayVector);
    const float rayTMin = 0.001;
    const float targetBias = 0.01;
    float rayTMax = rayLength - targetBias;

    if (rayTMax <= rayTMin) {
        return 1.0;
    }

    vec3 rayDirection = rayVector / rayLength;
    return TraceAnySceneHit(accelerationStructure, context, rayOrigin, rayDirection, rayTMin, rayTMax) ? 0.0 : 1.0;
}

#endif
