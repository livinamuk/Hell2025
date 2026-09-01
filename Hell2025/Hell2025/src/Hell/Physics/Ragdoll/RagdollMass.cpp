#include "RagdollMass.h"

#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <vector>

namespace RagdollMass {
    namespace {
        constexpr double PI = 3.14159265358979323846;
        constexpr double MINIMUM_MASS = 0.000001;
        constexpr double MINIMUM_VOLUME = 0.000000000001;

        float ComputeConvexVolume(const RagdollShape& shape) {
            if (shape.convexVertices.empty() || shape.convexIndices.size() < 3 || shape.convexIndices.size() % 3 != 0) return 0.0f;

            glm::dvec3 reference(0.0);
            for (const glm::vec3& vertex : shape.convexVertices) {
                reference += glm::dvec3(vertex);
            }
            reference /= static_cast<double>(shape.convexVertices.size());

            double volume = 0.0;
            for (size_t index = 0; index < shape.convexIndices.size(); index += 3) {
                const uint32_t index0 = shape.convexIndices[index + 0];
                const uint32_t index1 = shape.convexIndices[index + 1];
                const uint32_t index2 = shape.convexIndices[index + 2];
                if (index0 >= shape.convexVertices.size() || index1 >= shape.convexVertices.size() || index2 >= shape.convexVertices.size()) return 0.0f;

                const glm::dvec3 vertex0 = glm::dvec3(shape.convexVertices[index0]);
                const glm::dvec3 vertex1 = glm::dvec3(shape.convexVertices[index1]);
                const glm::dvec3 vertex2 = glm::dvec3(shape.convexVertices[index2]);
                volume += std::abs(glm::dot(glm::cross(vertex1 - vertex0, vertex2 - vertex0), reference - vertex0)) / 6.0;
            }
            return static_cast<float>(volume);
        }
    }

    float ComputeShapeVolume(const RagdollShape& shape) {
        switch (shape.type) {
            case RagdollShapeType::BOX: {
                const glm::dvec3 dimensions = glm::abs(glm::dvec3(shape.extents));
                return static_cast<float>(dimensions.x * dimensions.y * dimensions.z);
            }
            case RagdollShapeType::SPHERE: {
                const double radius = std::abs(static_cast<double>(shape.radius));
                return static_cast<float>((4.0 / 3.0) * PI * radius * radius * radius);
            }
            case RagdollShapeType::CAPSULE: {
                const double radius = std::abs(static_cast<double>(shape.radius));
                if (shape.length <= 0.0f) {
                    return static_cast<float>((4.0 / 3.0) * PI * radius * radius * radius);
                }

                const double cylinderLength = std::abs(static_cast<double>(shape.length));
                const double cylinderVolume = PI * radius * radius * cylinderLength;
                const double sphereVolume = (4.0 / 3.0) * PI * radius * radius * radius;
                return static_cast<float>(cylinderVolume + sphereVolume);
            }
            case RagdollShapeType::CONVEX_HULL:
                return ComputeConvexVolume(shape);
        }
        return 0.0f;
    }

    float ComputeMarkerVolume(const RagdollMarkerAsset& marker) {
        return marker.rigidBody.isKinematic ? 0.0f : ComputeShapeVolume(marker.shape);
    }

    float ComputeTotalMass(const RagdollAsset& asset) {
        double totalMass = 0.0;
        for (const RagdollMarkerAsset& marker : asset.markers) {
            if (!marker.rigidBody.isKinematic && std::isfinite(marker.rigidBody.mass) && marker.rigidBody.mass > 0.0f) {
                totalMass += marker.rigidBody.mass;
            }
        }
        return static_cast<float>(totalMass);
    }

    bool Recalculate(RagdollAsset& asset, std::string& error) {
        if (!std::isfinite(asset.targetMass) || asset.targetMass <= MINIMUM_MASS) {
            error = "Target total mass must be greater than zero";
            return false;
        }

        std::vector<float> volumes(asset.markers.size(), 0.0f);
        double overrideMass = 0.0;
        double automaticVolume = 0.0;
        size_t automaticCount = 0;

        for (size_t markerIndex = 0; markerIndex < asset.markers.size(); markerIndex++) {
            const RagdollMarkerAsset& marker = asset.markers[markerIndex];
            if (marker.rigidBody.isKinematic) continue;

            if (marker.rigidBody.massMode == RagdollMassMode::OVERRIDE) {
                if (!std::isfinite(marker.rigidBody.mass) || marker.rigidBody.mass <= MINIMUM_MASS) {
                    error = "Marker '" + marker.name + "' has an invalid mass override";
                    return false;
                }
                overrideMass += marker.rigidBody.mass;
                continue;
            }

            const float volume = ComputeMarkerVolume(marker);
            if (!std::isfinite(volume) || volume <= MINIMUM_VOLUME) {
                error = "Marker '" + marker.name + "' has no usable collision volume";
                return false;
            }
            volumes[markerIndex] = volume;
            automaticVolume += volume;
            automaticCount++;
        }

        const double remainingMass = static_cast<double>(asset.targetMass) - overrideMass;
        if (automaticCount == 0) {
            error.clear();
            return true;
        }
        if (remainingMass <= MINIMUM_MASS * static_cast<double>(automaticCount)) {
            error = "Mass overrides leave no usable mass for automatic markers";
            return false;
        }
        if (automaticVolume <= MINIMUM_VOLUME) {
            error = "Automatic markers have no usable collision volume";
            return false;
        }

        std::vector<float> masses(asset.markers.size(), 0.0f);
        double assignedAutomaticMass = 0.0;
        size_t automaticMarkersRemaining = automaticCount;
        for (size_t markerIndex = 0; markerIndex < asset.markers.size(); markerIndex++) {
            if (volumes[markerIndex] <= 0.0f) continue;
            automaticMarkersRemaining--;
            const double mass = automaticMarkersRemaining == 0
                ? remainingMass - assignedAutomaticMass
                : remainingMass * static_cast<double>(volumes[markerIndex]) / automaticVolume;
            if (!std::isfinite(mass) || mass <= MINIMUM_MASS) {
                error = "Target total mass produces an unusably small marker mass";
                return false;
            }
            masses[markerIndex] = static_cast<float>(mass);
            assignedAutomaticMass += masses[markerIndex];
        }

        for (size_t markerIndex = 0; markerIndex < asset.markers.size(); markerIndex++) {
            if (masses[markerIndex] > 0.0f) asset.markers[markerIndex].rigidBody.mass = masses[markerIndex];
        }
        error.clear();
        return true;
    }
}
