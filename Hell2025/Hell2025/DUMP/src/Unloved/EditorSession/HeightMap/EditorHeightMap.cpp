#include "EditorHeightMap.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/EditorSession/Core/EditorWorkspace.h"

#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Input.h"
#include "Hell/Time/Time.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Systems/HeightMap/HeightMap.h"
#include "Unloved/Systems/Map/MapManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace Unloved::EditorSession::HeightMapEditor {
    namespace {
        constexpr float MAX_RAY_DISTANCE = 2000.0f;
        constexpr float RAY_EPSILON = 0.0001f;
        constexpr float SURFACE_EPSILON = 0.001f;
        constexpr float MAX_BRUSH_SPEED = 12.0f;
        constexpr int32_t RAY_REFINEMENT_STEPS = 12;
        constexpr int32_t DEFAULT_BRUSH_RESOLUTION = 1024;
        constexpr const char* BRUSH_DIRECTORY = "res/textures/heightmap_brushes/";
        constexpr const char* BRUSH_TEXTURE_PREFIX = "HeightMapBrush_";

        struct BrushDefinition {
            const char* name;
            const char* filename;
        };

        constexpr std::array<BrushDefinition, static_cast<size_t>(BrushType::COUNT)> BRUSH_DEFINITIONS = {{
            { "Acrylic 1",    "acrylic1.exr" },
            { "Circle 0",     "circle0.exr" },
            { "Circle 1",     "circle1.exr" },
            { "Circle 2",     "circle2.exr" },
            { "Circle 3",     "circle3.exr" },
            { "Circle 4",     "circle4.exr" },
            { "Hill 1",       "hill1.exr" },
            { "Hill 2",       "hill2.exr" },
            { "Mountain 1",   "mountain1.exr" },
            { "Mountain 2",   "mountain2.exr" },
            { "Mountain 3",   "mountain3.exr" },
            { "Mountain 4",   "mountain4.exr" },
            { "Peak 1",       "peak1.exr" },
            { "Peak 2",       "peak2.exr" },
            { "Peak 3",       "peak3.exr" },
            { "Ring 1",       "ring1.exr" },
            { "Smoke",        "smoke.exr" },
            { "Square 1",     "square1.exr" },
            { "Square 2",     "square2.exr" },
            { "Square 3",     "square3.exr" },
            { "Square 4",     "square4.exr" },
            { "Square 5",     "square5.exr" },
            { "Stones",       "stones.exr" },
            { "Terrain 1",    "terrain1.exr" },
            { "Terrain 2",    "terrain2.exr" },
            { "Terrain 3",    "terrain3.exr" },
            { "Terrain 4",    "terrain4.exr" },
            { "Terrain 5",    "terrain5.exr" },
            { "Terrain 6",    "terrain6.exr" },
            { "Texture 1",    "texture1.exr" },
            { "Texture 2",    "texture2.exr" },
            { "Texture 3",    "texture3.exr" },
            { "Texture 4",    "texture4.exr" },
            { "Texture 5",    "texture5.exr" },
            { "Vegetation 1", "vegetation1.exr" },
        }};

        bool g_active = false;
        Tool g_tool = Tool::ADD;
        bool g_terrainLayersOpen = false;
        bool g_strokeActive = false;
        bool g_strokeChangesHeight = false;
        bool g_brushLoadAttempted = false;
        float g_brushSize = 8.0f;
        float g_brushStrength = 33.0f;
        float g_targetHeight = MapData::DEFAULT_HEIGHT;
        float g_brushRotation = 0.0f;
        float g_brushGamma = 1.0f;
        BrushType g_brushType = BrushType::CIRCLE_0;
        uint8_t g_selectedTerrainMaterial = 0;
        BrushPreview g_brushPreview;
        std::vector<float> g_brushData;
        int32_t g_brushWidth = 0;
        int32_t g_brushHeight = 0;

        struct TerrainHit {
            bool found = false;
            glm::vec3 position = glm::vec3(0.0f);
        };

        struct PaintBounds {
            int32_t minimumX = std::numeric_limits<int32_t>::max();
            int32_t minimumZ = std::numeric_limits<int32_t>::max();
            int32_t maximumX = std::numeric_limits<int32_t>::min();
            int32_t maximumZ = std::numeric_limits<int32_t>::min();

            bool Changed() const { return minimumX <= maximumX && minimumZ <= maximumZ; }
            void Include(int32_t x, int32_t z) {
                minimumX = std::min(minimumX, x);
                minimumZ = std::min(minimumZ, z);
                maximumX = std::max(maximumX, x);
                maximumZ = std::max(maximumZ, z);
            }
        };

        bool HasValidHeightData(const MapData& mapData) {
            const size_t expectedSize = static_cast<size_t>(mapData.GetTextureWidth()) * mapData.GetTextureHeight();
            return mapData.GetTextureWidth() > 0 && mapData.GetTextureHeight() > 0 && mapData.GetHeightMapData().size() == expectedSize;
        }

        bool HasValidTerrainControlData(const MapData& mapData) {
            const size_t expectedSize = static_cast<size_t>(mapData.GetTextureWidth()) * mapData.GetTextureHeight();
            return mapData.GetTerrainControlData().size() == expectedSize;
        }

        glm::vec2 WorldToTexel(const glm::vec3& worldPosition) {
            return glm::vec2(worldPosition.x, worldPosition.z) / HEIGHTMAP_SCALE_XZ;
        }

        glm::vec3 TexelToWorld(const glm::vec2& texelPosition, float height) {
            return glm::vec3(texelPosition.x * HEIGHTMAP_SCALE_XZ, height, texelPosition.y * HEIGHTMAP_SCALE_XZ);
        }

        float GetTexelHeight(const MapData& mapData, int32_t x, int32_t z) {
            x = std::clamp(x, 0, mapData.GetTextureWidth() - 1);
            z = std::clamp(z, 0, mapData.GetTextureHeight() - 1);
            return mapData.GetHeightMapData()[static_cast<size_t>(z) * mapData.GetTextureWidth() + x];
        }

        double CubicKernel(double value) {
            value = std::abs(value);
            if (value <= 1.0) return (1.5 * value - 2.5) * value * value + 1.0;
            if (value < 2.0) return ((-0.5 * value + 2.5) * value - 4.0) * value + 2.0;
            return 0.0;
        }

        void LoadBrush() {
            if (g_brushLoadAttempted) return;
            g_brushLoadAttempted = true;
            const size_t brushIndex = static_cast<size_t>(g_brushType);
            if (brushIndex >= BRUSH_DEFINITIONS.size()) return;
            const std::string brushPath = std::string(BRUSH_DIRECTORY) + BRUSH_DEFINITIONS[brushIndex].filename;
            const ImageData image = Hell::ImageTools::LoadEXRImage(brushPath);
            if (image.format != ImageFormat::RGBA32_SFLOAT || image.mips.empty()) return;

            const TextureMip& sourceMip = image.mips[0];
            const float* sourcePixels = reinterpret_cast<const float*>(sourceMip.data.data());
            if (sourceMip.width >= DEFAULT_BRUSH_RESOLUTION || sourceMip.height >= DEFAULT_BRUSH_RESOLUTION) {
                g_brushWidth = static_cast<int32_t>(sourceMip.width);
                g_brushHeight = static_cast<int32_t>(sourceMip.height);
                g_brushData.resize(static_cast<size_t>(g_brushWidth) * g_brushHeight);
                for (size_t i = 0; i < g_brushData.size(); i++) {
                    g_brushData[i] = sourcePixels[i * 4];
                }
                return;
            }

            // Upscale small brushes with bicubic sampling
            g_brushWidth = DEFAULT_BRUSH_RESOLUTION;
            g_brushHeight = DEFAULT_BRUSH_RESOLUTION;
            g_brushData.resize(static_cast<size_t>(g_brushWidth) * g_brushHeight);
            const double xScale = static_cast<double>(sourceMip.width) / g_brushWidth;
            const double yScale = static_cast<double>(sourceMip.height) / g_brushHeight;
            for (int32_t y = 0; y < g_brushHeight; y++) {
                const double sourceY = (static_cast<double>(y) + 0.5) * yScale - 0.5;
                const int32_t sourceY0 = static_cast<int32_t>(sourceY);
                const double fractionY = sourceY - sourceY0;
                for (int32_t x = 0; x < g_brushWidth; x++) {
                    const double sourceX = (static_cast<double>(x) + 0.5) * xScale - 0.5;
                    const int32_t sourceX0 = static_cast<int32_t>(sourceX);
                    const double fractionX = sourceX - sourceX0;
                    double value = 0.0;
                    for (int32_t sampleY = -1; sampleY < 3; sampleY++) {
                        const double weightY = CubicKernel(fractionY - sampleY);
                        const int32_t clampedY = std::clamp(sourceY0 + sampleY, 0, static_cast<int32_t>(sourceMip.height) - 1);
                        for (int32_t sampleX = -1; sampleX < 3; sampleX++) {
                            const double weight = weightY * CubicKernel(sampleX - fractionX);
                            const int32_t clampedX = std::clamp(sourceX0 + sampleX, 0, static_cast<int32_t>(sourceMip.width) - 1);
                            value += sourcePixels[(static_cast<size_t>(clampedY) * sourceMip.width + clampedX) * 4] * weight;
                        }
                    }
                    g_brushData[static_cast<size_t>(y) * g_brushWidth + x] = static_cast<float>(value);
                }
            }
        }

        float SampleBrush(glm::vec2 uv) {
            if (g_brushData.empty()) return 0.0f;

            const float rotation = glm::radians(g_brushRotation);
            const float cosine = std::cos(rotation);
            const float sine = std::sin(rotation);
            uv -= glm::vec2(0.5f);
            uv = glm::vec2(cosine * uv.x - sine * uv.y, sine * uv.x + cosine * uv.y) + glm::vec2(0.5f);
            uv = glm::clamp(uv, glm::vec2(0.0f), glm::vec2(1.0f));

            const int32_t x = static_cast<int32_t>(uv.x * g_brushWidth);
            const int32_t y = static_cast<int32_t>(uv.y * g_brushHeight);
            if (x < 0 || y < 0 || x >= g_brushWidth || y >= g_brushHeight) return 0.0f;
            return g_brushData[static_cast<size_t>(y) * g_brushWidth + x];
        }

        float SampleHeight(const MapData& mapData, const glm::vec3& worldPosition) {
            const glm::vec2 texelPosition = WorldToTexel(worldPosition);
            const float x = std::clamp(texelPosition.x, 0.0f, static_cast<float>(mapData.GetTextureWidth() - 1));
            const float z = std::clamp(texelPosition.y, 0.0f, static_cast<float>(mapData.GetTextureHeight() - 1));
            const int32_t x0 = static_cast<int32_t>(std::floor(x));
            const int32_t z0 = static_cast<int32_t>(std::floor(z));
            const int32_t x1 = std::min(x0 + 1, mapData.GetTextureWidth() - 1);
            const int32_t z1 = std::min(z0 + 1, mapData.GetTextureHeight() - 1);
            const float blendX = x - x0;
            const float blendZ = z - z0;
            const float height0 = glm::mix(GetTexelHeight(mapData, x0, z0), GetTexelHeight(mapData, x1, z0), blendX);
            const float height1 = glm::mix(GetTexelHeight(mapData, x0, z1), GetTexelHeight(mapData, x1, z1), blendX);
            return glm::mix(height0, height1, blendZ);
        }

        bool ClipRayAxis(float origin, float direction, float axisMaximum, float& enterDistance, float& exitDistance) {
            if (std::abs(direction) < RAY_EPSILON) return origin >= 0.0f && origin <= axisMaximum;
            float firstDistance = -origin / direction;
            float secondDistance = (axisMaximum - origin) / direction;
            if (firstDistance > secondDistance) {
                std::swap(firstDistance, secondDistance);
            }
            enterDistance = std::max(enterDistance, firstDistance);
            exitDistance = std::min(exitDistance, secondDistance);
            return enterDistance <= exitDistance;
        }

        bool ClipRayToMap(const MapData& mapData, const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float& enterDistance, float& exitDistance) {
            enterDistance = 0.0f;
            exitDistance = MAX_RAY_DISTANCE;
            const float worldWidth = mapData.GetTextureWidth() * HEIGHTMAP_SCALE_XZ;
            const float worldDepth = mapData.GetTextureHeight() * HEIGHTMAP_SCALE_XZ;
            return ClipRayAxis(rayOrigin.x, rayDirection.x, worldWidth, enterDistance, exitDistance) && ClipRayAxis(rayOrigin.z, rayDirection.z, worldDepth, enterDistance, exitDistance);
        }

        TerrainHit CreateTerrainHit(const MapData& mapData, const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float distance) {
            const glm::vec3 rayPosition = rayOrigin + rayDirection * distance;
            TerrainHit hit;
            hit.found = true;
            hit.position = TexelToWorld(WorldToTexel(rayPosition), SampleHeight(mapData, rayPosition));
            return hit;
        }

        TerrainHit Raycast(const MapData& mapData, const glm::vec3& rayOrigin, const glm::vec3& unnormalizedRayDirection) {
            // Normalize the ray
            const float directionLength = glm::length(unnormalizedRayDirection);
            if (directionLength < RAY_EPSILON) return {};
            const glm::vec3 rayDirection = unnormalizedRayDirection / directionLength;

            // Clip the ray to the map
            float enterDistance = 0.0f;
            float exitDistance = 0.0f;
            if (!ClipRayToMap(mapData, rayOrigin, rayDirection, enterDistance, exitDistance)) return {};

            // Handle vertical rays
            const float horizontalSpeed = std::max(std::abs(rayDirection.x), std::abs(rayDirection.z));
            if (horizontalSpeed < RAY_EPSILON) {
                if (std::abs(rayDirection.y) < RAY_EPSILON) return {};
                const float distance = (SampleHeight(mapData, rayOrigin) - rayOrigin.y) / rayDirection.y;
                return distance >= enterDistance && distance <= exitDistance ? CreateTerrainHit(mapData, rayOrigin, rayDirection, distance) : TerrainHit{};
            }

            // March across the terrain
            const float stepDistance = HEIGHTMAP_SCALE_XZ * 0.5f / horizontalSpeed;
            float previousDistance = enterDistance;
            float previousDifference = (rayOrigin + rayDirection * previousDistance).y - SampleHeight(mapData, rayOrigin + rayDirection * previousDistance);
            if (std::abs(previousDifference) <= SURFACE_EPSILON) return CreateTerrainHit(mapData, rayOrigin, rayDirection, previousDistance);

            while (previousDistance < exitDistance) {
                const float currentDistance = std::min(previousDistance + stepDistance, exitDistance);
                const glm::vec3 currentPosition = rayOrigin + rayDirection * currentDistance;
                const float currentDifference = currentPosition.y - SampleHeight(mapData, currentPosition);
                const bool crossedSurface = (previousDifference < 0.0f && currentDifference > 0.0f) || (previousDifference > 0.0f && currentDifference < 0.0f);

                if (std::abs(currentDifference) <= SURFACE_EPSILON) return CreateTerrainHit(mapData, rayOrigin, rayDirection, currentDistance);
                if (crossedSurface) {
                    // Refine the surface crossing
                    float lowerDistance = previousDistance;
                    float upperDistance = currentDistance;
                    float lowerDifference = previousDifference;
                    for (int32_t step = 0; step < RAY_REFINEMENT_STEPS; step++) {
                        const float middleDistance = (lowerDistance + upperDistance) * 0.5f;
                        const glm::vec3 middlePosition = rayOrigin + rayDirection * middleDistance;
                        const float middleDifference = middlePosition.y - SampleHeight(mapData, middlePosition);
                        if ((lowerDifference < 0.0f) == (middleDifference < 0.0f)) {
                            lowerDistance = middleDistance;
                            lowerDifference = middleDifference;
                        }
                        else {
                            upperDistance = middleDistance;
                        }
                    }
                    return CreateTerrainHit(mapData, rayOrigin, rayDirection, (lowerDistance + upperDistance) * 0.5f);
                }

                previousDistance = currentDistance;
                previousDifference = currentDifference;
            }
            return {};
        }

        float GetBrushDirection(bool leftMouseDown, bool rightMouseDown) {
            if (g_tool != Tool::ADD) return 0.0f;
            if (rightMouseDown) return -1.0f;
            return leftMouseDown ? 1.0f : 0.0f;
        }

        template <typename PaintFunction>
        bool PaintBrushTexels(const MapData& mapData, const glm::vec3& brushCenter, PaintFunction paintFunction, PaintBounds* paintBounds = nullptr, bool paintZeroFalloff = false) {
            const glm::vec2 centerTexel = WorldToTexel(brushCenter);
            const float brushRadius = g_brushSize * 0.5f;
            const float radiusTexels = brushRadius / HEIGHTMAP_SCALE_XZ;
            const int32_t minimumX = std::max(0, static_cast<int32_t>(std::floor(centerTexel.x - radiusTexels)));
            const int32_t maximumX = std::min(mapData.GetTextureWidth() - 1, static_cast<int32_t>(std::ceil(centerTexel.x + radiusTexels)));
            const int32_t minimumZ = std::max(0, static_cast<int32_t>(std::floor(centerTexel.y - radiusTexels)));
            const int32_t maximumZ = std::min(mapData.GetTextureHeight() - 1, static_cast<int32_t>(std::ceil(centerTexel.y + radiusTexels)));
            bool changed = false;

            for (int32_t x = minimumX; x <= maximumX; x++) {
                for (int32_t z = minimumZ; z <= maximumZ; z++) {
                    const glm::vec2 offset = (glm::vec2(x, z) - centerTexel) * HEIGHTMAP_SCALE_XZ;
                    const glm::vec2 brushUv = offset / g_brushSize + glm::vec2(0.5f);
                    if (paintZeroFalloff && (brushUv.x < 0.0f || brushUv.y < 0.0f || brushUv.x >= 1.0f || brushUv.y >= 1.0f)) continue;

                    float falloff = std::pow(SampleBrush(brushUv), g_brushGamma);
                    falloff = std::isnan(falloff) ? 0.0f : falloff;
                    if (!std::isfinite(falloff) || (!paintZeroFalloff && falloff <= 0.0f)) continue;

                    if (paintFunction(static_cast<size_t>(z) * mapData.GetTextureWidth() + x, falloff)) {
                        changed = true;
                        if (paintBounds) {
                            paintBounds->Include(x, z);
                        }
                    }
                }
            }
            return changed;
        }

        template <typename PaintFunction>
        bool PaintHeightBrush(MapData& mapData, const glm::vec3& brushCenter, PaintFunction paintFunction) {
            std::vector<float>& heightData = mapData.GetHeightMapData();
            return PaintBrushTexels(mapData, brushCenter, [&](size_t index, float falloff) {
                float& height = heightData[index];
                const float newHeight = std::clamp(paintFunction(height, falloff), 0.0f, HEIGHTMAP_SCALE_Y);
                if (newHeight == height) return false;

                height = newHeight;
                return true;
            });
        }

        template <typename PaintFunction>
        PaintBounds PaintTerrainControlBrush(MapData& mapData, const glm::vec3& brushCenter, PaintFunction paintFunction) {
            std::vector<uint32_t>& controlData = mapData.GetTerrainControlData();
            PaintBounds paintBounds;
            PaintBrushTexels(mapData, brushCenter, [&](size_t index, float falloff) {
                uint32_t& control = controlData[index];
                const uint32_t newControl = paintFunction(control, falloff);
                if (newControl == control) return false;

                control = newControl;
                return true;
            }, &paintBounds);
            return paintBounds;
        }

        bool PaintRaiseLower(MapData& mapData, const glm::vec3& brushCenter, float brushDirection) {
            const float frameStrength = brushDirection * MAX_BRUSH_SPEED * g_brushStrength * 0.01f * Hell::Time::DeltaTime();
            return PaintHeightBrush(mapData, brushCenter, [frameStrength](float height, float falloff) { return height + frameStrength * falloff; });
        }

        bool PaintTargetHeight(MapData& mapData, const glm::vec3& brushCenter) {
            const float strength = std::clamp(g_brushStrength * 0.01f, 0.01f, 1000.0f);
            return PaintHeightBrush(mapData, brushCenter, [strength](float height, float brushAlpha) {
                const float blend = std::clamp(brushAlpha * strength, 0.0f, 1.0f);
                return height + (g_targetHeight - height) * blend;
            });
        }

        bool PaintSmooth(MapData& mapData, const glm::vec3& brushCenter) {
            std::vector<float>& heightData = mapData.GetHeightMapData();
            const int32_t width = mapData.GetTextureWidth();
            const int32_t height = mapData.GetTextureHeight();
            const float strength = std::clamp(g_brushStrength * 0.01f, 0.01f, 1000.0f);

            return PaintBrushTexels(mapData, brushCenter, [&](size_t index, float brushAlpha) {
                const int32_t x = static_cast<int32_t>(index % width);
                const int32_t z = static_cast<int32_t>(index / width);
                float sourceHeight = heightData[index];
                sourceHeight = std::isnan(sourceHeight) ? 0.0f : sourceHeight;

                const auto getNeighbourHeight = [&](int32_t neighbourX, int32_t neighbourZ) {
                    if (neighbourX < 0 || neighbourZ < 0 || neighbourX >= width || neighbourZ >= height) return sourceHeight;

                    const float neighbourHeight = heightData[static_cast<size_t>(neighbourZ) * width + neighbourX];
                    return std::isnan(neighbourHeight) ? sourceHeight : neighbourHeight;
                };

                const float averageHeight = (sourceHeight + getNeighbourHeight(x - 1, z) + getNeighbourHeight(x + 1, z) + getNeighbourHeight(x, z - 1) + getNeighbourHeight(x, z + 1)) * 0.2f;
                const float blend = std::clamp(brushAlpha * strength * 2.0f, 0.02f, 1.0f);
                const float newHeight = std::clamp(sourceHeight + (averageHeight - sourceHeight) * blend, 0.0f, HEIGHTMAP_SCALE_Y);
                if (newHeight == heightData[index]) return false;

                heightData[index] = newHeight;
                return true;
            }, nullptr, true);
        }

        PaintBounds PaintTexture(MapData& mapData, const glm::vec3& brushCenter) {
            return PaintTerrainControlBrush(mapData, brushCenter, [](uint32_t control, float falloff) { return falloff > 0.5f ? TerrainControl::Encode(g_selectedTerrainMaterial, g_selectedTerrainMaterial, 0, false) : control; });
        }

        PaintBounds SprayTexture(MapData& mapData, const glm::vec3& brushCenter) {
            const float strength = g_brushStrength * 0.01f;
            const float sprayStrength = std::clamp(strength * 0.05f, 0.004f, 0.25f);
            return PaintTerrainControlBrush(mapData, brushCenter, [strength, sprayStrength](uint32_t control, float brushAlpha) {
                // Decode the packed control
                uint8_t baseMaterial = TerrainControl::GetBaseMaterial(control);
                uint8_t overlayMaterial = TerrainControl::GetOverlayMaterial(control);
                float blend = static_cast<float>(TerrainControl::GetBlend(control)) / TerrainControl::BLEND_MASK;
                bool useAutoShader = TerrainControl::UsesAutoShader(control);
                const float brushValue = std::clamp(brushAlpha * sprayStrength, 0.0f, 1.0f);
                constexpr float replacementBlendThreshold = 1.0f / 254.0f;

                if (brushAlpha * strength * 11.0f > 0.1f) {
                    // Choose which layer to replace
                    if (g_selectedTerrainMaterial != baseMaterial && g_selectedTerrainMaterial != overlayMaterial) {
                        if (blend >= 0.5f) {
                            blend = std::min(1.0f, blend + brushValue);
                        }
                        else {
                            blend = std::max(0.0f, blend - brushValue);
                        }

                        if (blend <= replacementBlendThreshold) {
                            overlayMaterial = g_selectedTerrainMaterial;
                        }
                        else if (blend >= 1.0f - replacementBlendThreshold) {
                            baseMaterial = g_selectedTerrainMaterial;
                        }
                    }

                    // Move the blend and disable auto shader after takeover
                    if (baseMaterial == g_selectedTerrainMaterial) {
                        blend = std::max(0.0f, blend - brushValue);
                        if (brushAlpha > 0.5f && blend < 0.5f) {
                            useAutoShader = false;
                        }
                    }
                    if (overlayMaterial == g_selectedTerrainMaterial) {
                        blend = std::min(1.0f, blend + brushValue);
                        if (brushAlpha > 0.5f && blend >= 0.5f) {
                            useAutoShader = false;
                        }
                    }
                }

                // Re-encode the packed control
                const uint8_t quantizedBlend = static_cast<uint8_t>(std::round(blend * TerrainControl::BLEND_MASK));
                return TerrainControl::Encode(baseMaterial, overlayMaterial, quantizedBlend, useAutoShader);
            });
        }

        PaintBounds PaintAutoShader(MapData& mapData, const glm::vec3& brushCenter, bool enabled) {
            return PaintTerrainControlBrush(mapData, brushCenter, [enabled](uint32_t control, float falloff) {
                if (falloff <= 0.5f) return control;
                return enabled ? control | TerrainControl::AUTO_SHADER_MASK : control & ~TerrainControl::AUTO_SHADER_MASK;
            });
        }

        void RefreshHeightMap(MapData& mapData, bool heightChanged, bool updatePhysics) {
            HeightMap::BuildWorldHeightData(mapData.GetChunkCountX(), mapData.GetChunkCountZ());
            if (heightChanged) {
                Renderer::RecalculateAllHeightMapData(true, updatePhysics);
            }
            else {
                OpenGL::Renderer::UploadWorldHeightData();
            }
        }

        void FinishStroke() {
            if (g_strokeChangesHeight) {
                Renderer::RecalculateAllHeightMapData(false, true);
            }
            g_strokeActive = false;
            g_strokeChangesHeight = false;
        }
    }

    void ResetTools() {
        g_active = false;
        g_tool = Tool::ADD;
        g_terrainLayersOpen = false;
    }

    void SetActive(bool active) {
        g_active = active;
    }

    void SetTool(Tool tool) {
        g_tool = tool;
    }

    void SetTerrainLayersOpen(bool open) {
        g_terrainLayersOpen = open;
    }

    void SetBrushSize(float value) {
        g_brushSize = std::clamp(value, 0.5f, 32.0f);
    }

    void SetBrushStrength(float value) {
        g_brushStrength = std::clamp(value, 1.0f, 100.0f);
    }

    void SetTargetHeight(float value) {
        g_targetHeight = std::clamp(value, 0.0f, HEIGHTMAP_SCALE_Y);
    }

    void SetBrushRotation(float value) {
        g_brushRotation = std::clamp(value, -180.0f, 180.0f);
    }

    void SetBrushGamma(float value) {
        g_brushGamma = std::clamp(value, 0.1f, 2.0f);
    }

    void SetBrushType(BrushType value) {
        if (value == g_brushType || static_cast<size_t>(value) >= BRUSH_DEFINITIONS.size()) return;
        g_brushType = value;
        g_brushLoadAttempted = false;
        g_brushData.clear();
        g_brushWidth = 0;
        g_brushHeight = 0;
    }

    void SetSelectedTerrainMaterial(uint8_t materialIndex) {
        g_selectedTerrainMaterial = std::min(materialIndex, static_cast<uint8_t>(TerrainControl::MATERIAL_MASK));
    }

    bool IsActive() {
        return g_active;
    }

    Tool GetTool() {
        return g_tool;
    }

    bool IsTerrainLayersOpen() {
        return g_terrainLayersOpen;
    }

    float GetBrushSize() {
        return g_brushSize;
    }

    float GetBrushStrength() {
        return g_brushStrength;
    }

    float GetTargetHeight() {
        return g_targetHeight;
    }

    float GetBrushRotation() {
        return g_brushRotation;
    }

    float GetBrushGamma() {
        return g_brushGamma;
    }

    BrushType GetBrushType() {
        return g_brushType;
    }

    const char* GetBrushTypeName(BrushType value) {
        const size_t brushIndex = static_cast<size_t>(value);
        return brushIndex < BRUSH_DEFINITIONS.size() ? BRUSH_DEFINITIONS[brushIndex].name : "Unknown";
    }

    std::string GetBrushTextureName(BrushType value) {
        const size_t brushIndex = static_cast<size_t>(value);
        if (brushIndex >= BRUSH_DEFINITIONS.size()) return "";
        std::string filename = BRUSH_DEFINITIONS[brushIndex].filename;
        const size_t extensionOffset = filename.find_last_of('.');
        if (extensionOffset != std::string::npos) {
            filename.resize(extensionOffset);
        }
        return std::string(BRUSH_TEXTURE_PREFIX) + filename;
    }

    uint8_t GetSelectedTerrainMaterial() {
        return g_selectedTerrainMaterial;
    }

    const BrushPreview& GetBrushPreview() {
        return g_brushPreview;
    }

    void Update(bool allowMouseInput) {
        g_brushPreview.visible = false;
        const bool leftMouseDown = Hell::Input::LeftMouseDown();
        const bool rightMouseDown = Hell::Input::RightMouseDown();

        // Finish released strokes
        if (g_strokeActive && !leftMouseDown && !rightMouseDown) {
            FinishStroke();
        }

        // Load the active brush when needed
        if (Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::MAP && g_active) {
            LoadBrush();
        }

        if (!allowMouseInput || !Workspace::HasMode() || Workspace::GetMode() != EditorSessionMode::MAP) {
            return;
        }

        // Find the terrain below the mouse
        const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
        if (viewportIndex < 0) {
            return;
        }

        MapData* mapData = MapManager::GetMapDataByName(Workspace::GetName());
        if (!mapData || !HasValidHeightData(*mapData)) {
            return;
        }

        const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(viewportIndex);
        const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(viewportIndex);
        TerrainHit hit = Raycast(*mapData, rayOrigin, rayDirection);

        // Capture the orbit point
        if (hit.found && (Hell::Input::LeftMousePressed() || Hell::Input::RightMousePressed())) {
            Viewports::SetOrbitPoint(viewportIndex, hit.position);
        }

        const Tool tool = g_tool;
        const bool controlDown = Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);

        // Capture the flat target
        if (hit.found && tool == Tool::FLAT && controlDown && Hell::Input::LeftMousePressed()) {
            SetTargetHeight(hit.position.y);
        }

        // Apply the selected brush
        bool heightPainted = false;
        PaintBounds terrainControlPaintBounds;
        const bool brushEvent = Hell::Input::LeftMousePressed() || Hell::Input::GetMouseOffsetX() != 0.0f || Hell::Input::GetMouseOffsetY() != 0.0f;
        if (hit.found && tool == Tool::ADD) {
            const float brushDirection = GetBrushDirection(leftMouseDown, rightMouseDown);
            if (brushDirection != 0.0f) {
                heightPainted = PaintRaiseLower(*mapData, hit.position, brushDirection);
            }
        }
        if (hit.found && tool == Tool::FLAT && leftMouseDown && brushEvent) {
            heightPainted = PaintTargetHeight(*mapData, hit.position);
        }
        if (hit.found && HasValidTerrainControlData(*mapData) && tool == Tool::TEXTURE_PAINT && leftMouseDown) {
            terrainControlPaintBounds = PaintTexture(*mapData, hit.position);
        }
        if (hit.found && tool == Tool::SMOOTH && leftMouseDown && brushEvent) {
            heightPainted = PaintSmooth(*mapData, hit.position);
        }
        if (hit.found && HasValidTerrainControlData(*mapData) && tool == Tool::TEXTURE_SPRAY && leftMouseDown && brushEvent) {
            terrainControlPaintBounds = SprayTexture(*mapData, hit.position);
        }
        if (hit.found && HasValidTerrainControlData(*mapData) && tool == Tool::AUTO_SHADER && (leftMouseDown || rightMouseDown)) {
            terrainControlPaintBounds = PaintAutoShader(*mapData, hit.position, !rightMouseDown);
        }

        // Upload changed terrain data
        if (heightPainted || terrainControlPaintBounds.Changed()) {
            g_strokeActive = true;
            g_strokeChangesHeight = g_strokeChangesHeight || heightPainted;
        }
        if (heightPainted) {
            RefreshHeightMap(*mapData, true, false);
        }
        if (terrainControlPaintBounds.Changed()) {
            OpenGL::Renderer::UploadTerrainControlData(mapData, terrainControlPaintBounds.minimumX, terrainControlPaintBounds.minimumZ, terrainControlPaintBounds.maximumX, terrainControlPaintBounds.maximumZ);
        }
        if (heightPainted) {
            hit = Raycast(*mapData, rayOrigin, rayDirection);
        }

        // Update the brush preview
        if (hit.found) {
            g_brushPreview.position = hit.position;
            g_brushPreview.radius = g_brushSize * 0.5f;
            g_brushPreview.viewportIndex = viewportIndex;
            g_brushPreview.visible = true;
        }
    }
}
