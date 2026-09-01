#pragma once
#include "AABB.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <limits>
#include <vector>

struct OBBRayResult {
	bool hitFound = false;
	float distanceToHit = std::numeric_limits<float>::max();
	glm::vec3 hitPositionWorld = glm::vec3(0.0f);
	glm::vec3 hitPositionLocal = glm::vec3(0.0f);
	glm::vec3 hitNormalWorld = glm::vec3(0.0f);
	glm::vec3 hitNormalLocal = glm::vec3(0.0f);
};

struct OBB {
	OBB() = default;
	OBB(const AABB& localBounds, const glm::mat4& worldMatrix);

	void SetTransform(const glm::mat4& worldMatrix);
	void SetLocalBounds(const AABB& localBounds);
	glm::vec3 ClosestPoint(const glm::vec3& point) const;
	OBBRayResult Raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance = std::numeric_limits<float>::max()) const;

	const AABB& GetLocalBounds() const               { return m_localBounds; }
	const glm::mat4& GetWorldTransform() const       { return m_worldTransform; }
	const std::vector<glm::vec3>& GetCorners() const { return m_corners; }

private:
	void RecomputeCorners();

	AABB m_localBounds;
	glm::mat4 m_worldTransform = glm::mat4(1.0f);
	std::vector<glm::vec3> m_corners;
};
