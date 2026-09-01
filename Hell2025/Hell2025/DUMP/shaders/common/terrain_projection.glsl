#ifndef TERRAIN_PROJECTION_GLSL
#define TERRAIN_PROJECTION_GLSL

bool GetTerrainTextureProjection(
    uint materialId,
    vec3 iVertex,
    vec3 iNormal,
    vec3 baseDdx,
    vec3 baseDdy,
    out vec2 iUv,
    out vec2 iDdx,
    out vec2 iDdy,
    out mat2 projectedNormalAlignment) {

    iUv = iVertex.xz;
    iDdx = baseDdx.xz;
    iDdy = baseDdy.xz;
    projectedNormalAlignment = mat2(1.0);
    bool verticallyProjected = false;

#if TERRAIN_VERTICAL_PROJECTION_ENABLED
    bool materialUsesVerticalProjection = bool((TERRAIN_VERTICAL_PROJECTION_MATERIAL_MASK >> materialId) & 0x1u);
    if (materialUsesVerticalProjection && iNormal.y <= 0.7071067811865475) { // sqrt(0.5)
        verticallyProjected = true;
        // Direct port of Terrain3D's projected normal-map alignment matrix.
        projectedNormalAlignment = mat2(vec2(iNormal.z, -iNormal.x), vec2(iNormal.x, iNormal.z));

        // Terrain3D's fast 45-degree snapping: https://iquilezles.org/articles/noatan/
        vec2 xz = round(normalize(-iNormal.xz) * 1.3065629648763765); // sqrt(1.0 + sqrt(0.5))
        xz *= abs(xz.x) + abs(xz.y) > 1.5 ? 0.7071067811865475 : 1.0; // sqrt(0.5)
        xz = vec2(-xz.y, xz.x);

        iUv = vec2(dot(iVertex.xz, xz), -iVertex.y);
        iDdx = vec2(dot(baseDdx.xz, xz), -baseDdx.y);
        iDdy = vec2(dot(baseDdy.xz, xz), -baseDdy.y);
    }
#endif

    return verticallyProjected;
}

#endif
