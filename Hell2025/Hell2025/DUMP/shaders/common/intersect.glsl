bool SphereVsAABB(vec3 aabbMin, vec3 aabbMax, vec3 sphereOrigin, float sphereRadius) {
    vec3 closestPoint = clamp(sphereOrigin, aabbMin, aabbMax);
    float distanceSquared = dot(closestPoint - sphereOrigin, closestPoint - sphereOrigin);
    return distanceSquared <= (sphereRadius * sphereRadius);
}

bool AABBsIntersect(vec3 minA, vec3 maxA, vec3 minB, vec3 maxB) {
    return !(minA.x > maxB.x || maxA.x < minB.x ||
             minA.y > maxB.y || maxA.y < minB.y ||
             minA.z > maxB.z || maxA.z < minB.z);
}

bool PointInAABB(vec3 point, vec3 aabbMin, vec3 aabbMax) {
    return !(point.x < aabbMin.x || point.x > aabbMax.x ||
             point.y < aabbMin.y || point.y > aabbMax.y ||
             point.z < aabbMin.z || point.z > aabbMax.z);
}

bool PointInAABBHalfExtents(vec3 point, vec3 center, vec3 halfSize) {
    return all(lessThanEqual(abs(point - center), halfSize));
}
