#include "types.glsl"

mat4 ToMat4(vec3 position, vec3 rotation, vec3 scale) {
    // Translation matrix
    mat4 translationMatrix = mat4(1.0);
    translationMatrix[3] = vec4(position, 1.0);

    // Rotation matrices (XYZ Euler Order)
    float cosX = cos(rotation.x), sinX = sin(rotation.x);
    float cosY = cos(rotation.y), sinY = sin(rotation.y);
    float cosZ = cos(rotation.z), sinZ = sin(rotation.z);

    mat4 rotX = mat4(
        1,  0,    0,   0,
        0,  cosX, -sinX, 0,
        0,  sinX, cosX, 0,
        0,  0,    0,   1
    );

    mat4 rotY = mat4(
        cosY,  0, sinY,  0,
        0,     1, 0,     0,
        -sinY, 0, cosY,  0,
        0,     0, 0,     1
    );

    mat4 rotZ = mat4(
        cosZ, -sinZ, 0, 0,
        sinZ, cosZ,  0, 0,
        0,    0,     1, 0,
        0,    0,     0, 1
    );

    // Combined rotation (Z * Y * X order)
    mat4 rotationMatrix = rotZ * rotY * rotX;

    // Scale matrix
    mat4 scaleMatrix = mat4(1.0);
    scaleMatrix[0][0] = scale.x;
    scaleMatrix[1][1] = scale.y;
    scaleMatrix[2][2] = scale.z;

    // Final transformation matrix
    return translationMatrix * rotationMatrix * scaleMatrix;
}

float RandOLD(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float Rand(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

float LinearizeDepth(float nonLinearDepth, float near, float far) {
    float z = nonLinearDepth * 2.0 - 1.0;  // Convert [0,1] range to [-1,1] (NDC space)
    return (2.0 * near * far) / (far + near - z * (far - near)); // Convert to linear depth
}

// Rotation matrix around the X axis.
mat3 RotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

// Rotation matrix around the Y axis
mat3 RotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}

// Rotation matrix around the Z axis
mat3 RotateZ(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    );
}

bool RaySphereHit(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius) {
    vec3 originToCenter = sphereCenter - rayOrigin;
    float radiusSquared = sphereRadius * sphereRadius;

    float centerDistanceSquared = dot(originToCenter, originToCenter);
    float closestApproachT = dot(originToCenter, rayDir);

    if (closestApproachT < 0.0 && centerDistanceSquared > radiusSquared) return false;

    float perpendicularDistanceSquared = centerDistanceSquared - closestApproachT * closestApproachT;
    if (perpendicularDistanceSquared > radiusSquared) return false;

    return true;
}

bool RaySphereHit(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius, out float tHit) {
    vec3 oc = rayOrigin - sphereCenter;
    float b = dot(oc, rayDir);
    float cTerm = dot(oc, oc) - sphereRadius * sphereRadius;
    float h = b * b - cTerm;
    if (h < 0.0) return false;
    float s = sqrt(h);
    float t0 = -b - s;
    float t1 = -b + s;
    tHit = (t0 > 0.0) ? t0 : t1;
    return tHit > 0.0;
}

bool PointInAABBBounds(vec3 point, vec3 aabbMin, vec3 aabbMax) {
    return all(greaterThanEqual(point, aabbMin)) && all(lessThanEqual(point, aabbMax));
}

bool PointInSphere(vec3 p, vec3 center, float radius) {
    vec3 diff = p - center;
    float distSq = dot(diff, diff);
    return distSq <= (radius * radius);
}
