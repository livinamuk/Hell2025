#ifndef VIEWPORT_GLSL
#define VIEWPORT_GLSL

const int VIEWPORT_LAYOUT_SINGLE = 0;
const int VIEWPORT_LAYOUT_COLUMNS = 1;
const int VIEWPORT_LAYOUT_ROWS = 2;
const int VIEWPORT_LAYOUT_GRID = 3;

vec2 ScreenUVFromPixel(ivec2 renderPixel, ivec2 outputImageSize) {
    return (vec2(renderPixel) + 0.5) / vec2(outputImageSize);
}

vec2 ScreenUVFromFragCoord(vec2 fragCoord, ivec2 outputImageSize) {
    return fragCoord / vec2(outputImageSize);
}

ivec2 ScreenPixelFromRenderPixel(ivec2 renderPixel, ivec2 outputImageSize) {
#if defined(VULKAN)
    return ivec2(renderPixel.x, outputImageSize.y - 1 - renderPixel.y);
#else
    return renderPixel;
#endif
}

#if defined(VULKAN)
ivec4 VulkanViewportRectFromScreenRect(ivec4 screenRect, ivec2 outputImageSize) {
    return ivec4(screenRect.x, outputImageSize.y - screenRect.y - screenRect.w, screenRect.z, screenRect.w);
}
#endif

uint ViewportIndexFromPixel(ivec2 renderPixel, ivec2 outputImageSize, int viewportLayout, vec2 viewportSplit) {
    ivec2 screenPixel = ScreenPixelFromRenderPixel(renderPixel, outputImageSize);
    ivec2 splitPixel = ivec2(viewportSplit * vec2(outputImageSize));
    if (viewportLayout == VIEWPORT_LAYOUT_SINGLE)  return 0u;
    if (viewportLayout == VIEWPORT_LAYOUT_COLUMNS) return uint(screenPixel.x >= splitPixel.x);
    if (viewportLayout == VIEWPORT_LAYOUT_ROWS)    return uint(screenPixel.y >= splitPixel.y);
    if (viewportLayout == VIEWPORT_LAYOUT_GRID)    return uint(screenPixel.x >= splitPixel.x) + uint(screenPixel.y >= splitPixel.y) * 2u;
    return 0u;
}

bool IsViewportActive(uint viewportIndex, uint activeViewportMask) {
    return (activeViewportMask & (1u << viewportIndex)) != 0u;
}

bool ViewportContainsPixel(ivec2 renderPixel, ivec2 outputImageSize, ivec4 viewportRect) {
    ivec2 screenPixel = ScreenPixelFromRenderPixel(renderPixel, outputImageSize);
    return all(greaterThanEqual(screenPixel, viewportRect.xy)) && all(lessThan(screenPixel, viewportRect.xy + viewportRect.zw));
}

vec2 ViewportUVFromPixel(ivec2 renderPixel, ivec2 outputImageSize, ivec4 viewportRect) {
    ivec2 screenPixel = ScreenPixelFromRenderPixel(renderPixel, outputImageSize);
    vec2 viewportPosition = vec2(screenPixel) + 0.5 - vec2(viewportRect.xy);
    return viewportPosition / vec2(viewportRect.zw);
}

vec2 ViewportUVFromScreenUV(vec2 screenUV, ivec2 outputImageSize, ivec4 viewportRect) {
    vec2 screenPosition = screenUV * vec2(outputImageSize);
#if defined(VULKAN)
    screenPosition.y = float(outputImageSize.y) - screenPosition.y;
#endif
    return (screenPosition - vec2(viewportRect.xy)) / vec2(viewportRect.zw);
}

vec2 ScreenUVFromViewportUV(vec2 viewportUV, ivec2 outputImageSize, ivec4 viewportRect) {
    vec2 screenPosition = vec2(viewportRect.xy) + viewportUV * vec2(viewportRect.zw);
#if defined(VULKAN)
    screenPosition.y = float(outputImageSize.y) - screenPosition.y;
#endif
    return screenPosition / vec2(outputImageSize);
}

vec2 ViewportNDCFromViewportUV(vec2 viewportUV) {
    return vec2(viewportUV.x * 2.0 - 1.0, 1.0 - viewportUV.y * 2.0);
}

vec2 ViewportUVFromClipPosition(vec4 clipPosition) {
    vec2 viewportNDC = clipPosition.xy / clipPosition.w;
    return vec2(viewportNDC.x * 0.5 + 0.5, 0.5 - viewportNDC.y * 0.5);
}

vec2 ViewportNDCFromPixel(ivec2 renderPixel, ivec2 outputImageSize, ivec4 viewportRect) {
    vec2 viewportUV = ViewportUVFromPixel(renderPixel, outputImageSize, viewportRect);
    return ViewportNDCFromViewportUV(viewportUV);
}

vec3 WorldRayFromPixel(ivec2 renderPixel, ivec2 outputImageSize, ivec4 viewportRect, vec3 viewPosition, mat4 inverseProjectionView) {
    vec2 viewportNDC = ViewportNDCFromPixel(renderPixel, outputImageSize, viewportRect);
    vec4 worldH = inverseProjectionView * vec4(viewportNDC, 1.0, 1.0);
    vec3 worldPosition = worldH.xyz / worldH.w;
    return normalize(worldPosition - viewPosition);
}

#endif
