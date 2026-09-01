#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

#include "../common/reconstruction.glsl"
#include "../common/util.glsl"
#include "../common/normal_encoding.glsl"
#include "../common/viewport.glsl"

layout(location = 0) out vec4 BaseColorMetallicOut;
layout(location = 1) out vec4 NormalXYRoughnessMiscOut;
layout(location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

layout(binding = 0) uniform sampler2D u_decalTexture;
layout(binding = 1) uniform sampler2D u_depthTexture;

layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) readonly restrict buffer viewportDataBuffer {
    ViewportData viewportData[];
};

uniform int u_viewportIndex;
uniform mat4 u_inverseModelMatrix;

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 gBufferSize = textureSize(u_depthTexture, 0);

    float depth = texelFetch(u_depthTexture, px, 0).r;
    if (depth <= 0.0) {
        discard;
    }

    ivec4 viewportRect = ivec4(viewportData[u_viewportIndex].xOffset, viewportData[u_viewportIndex].yOffset, viewportData[u_viewportIndex].width, viewportData[u_viewportIndex].height);
    mat4 inverseProjectionView = viewportData[u_viewportIndex].inverseJitteredProjectionViewReverseZ;
    vec2 viewportUV = ViewportUVFromPixel(px, gBufferSize, viewportRect);
    vec3 worldPosition = WorldPosFromDepth(viewportUV, depth, inverseProjectionView);

    vec3 positionDx = dFdx(worldPosition);
    vec3 positionDy = dFdy(worldPosition);
    vec3 receiverNormalUnnormalized = cross(positionDx, positionDy);
    float receiverNormalLengthSquared = dot(receiverNormalUnnormalized, receiverNormalUnnormalized);

    // The derivative magnitude changes with distance from the camera, so only
    // reject a genuinely degenerate or non-finite surface differential
    if (!(receiverNormalLengthSquared > 0.0) || isinf(receiverNormalLengthSquared)) {
        discard;
    }

    vec3 receiverNormal = receiverNormalUnnormalized * inversesqrt(receiverNormalLengthSquared);

    const float decalDepthScale = 0.2;

    vec2 decalTextureSize = vec2(textureSize(u_decalTexture, 0));
    float shortestTextureSide = max(min(decalTextureSize.x, decalTextureSize.y), 1.0);
    vec2 decalAspectScale = decalTextureSize / shortestTextureSide;

    vec3 localPosition = (u_inverseModelMatrix * vec4(worldPosition, 1.0)).xyz;
    vec3 decalHalfExtents = vec3(0.5 * decalAspectScale, 0.5 * decalDepthScale);
    if (any(greaterThan(abs(localPosition), decalHalfExtents))) {
        discard;
    }

    vec3 decalNormal = normalize(
        transpose(mat3(u_inverseModelMatrix)) * vec3(0.0, 0.0, 1.0)
    );

    float normalAlignment = dot(receiverNormal, decalNormal);
    if (normalAlignment < 0.0) {
        receiverNormal = -receiverNormal;
        normalAlignment = -normalAlignment;
    }

    const float minimumNormalAlignment = 0.0871557; // cos(85 degrees)
    if (normalAlignment < minimumNormalAlignment) {
        discard;
    }

    // Local Z follows the hit normal, so the decal lies in the local XY plane
    vec2 decalUV = (localPosition.xy / decalAspectScale) + 0.5;
    decalUV.y = 1 - decalUV.y;

    vec4 decalColor = texture(u_decalTexture, decalUV);

    if (decalColor.a < 0.5) {
        discard;
    }

    vec3 bloodBaseColor = vec3(0.2, 0.00, 0);
    const float roughness = 0.125;
    const float metallic = 0.25;
    const float ao = 1.0;

    BaseColorMetallicOut = vec4(decalColor.rgb, metallic);
    NormalXYRoughnessMiscOut = vec4(EncodeOct(receiverNormal), roughness, 0.0);
    VelocityXYOcclusionSubSurfaceOut = vec4(0.0, 0.0, ao, 0.0);
}
