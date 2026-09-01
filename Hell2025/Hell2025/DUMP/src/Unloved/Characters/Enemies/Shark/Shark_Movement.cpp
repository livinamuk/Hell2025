#include "Shark.h"

#include "Hell/Input.h"
#include "Hell/Math/LineMath.h"
#include "Hell/Physics/Ragdoll/Ragdoll.h"

#include "Unloved/Session/Session.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace Input = Hell::Input;

namespace Unloved {
    namespace {
        glm::vec3 NormalizeXZOr(const glm::vec3& v, const glm::vec3& fallback) {
            glm::vec3 out = v;
            out.y = 0.0f;
            float lenSq = glm::dot(out, out);
            if (lenSq > 0.000001f) return out / std::sqrt(lenSq);
            return fallback;
        }

        // Forward-only (wrap) scan starting at startIndex. Returns an index into path.
        // Returns startIndex if nothing qualifies as "in front" (so it never goes backwards).
        int GetClosestPointNotBehindSharkIndexForwardOnly(const std::vector<glm::vec3>& path, int startIndex, const glm::vec3& currentPosition, const glm::vec3& currentForward, float dotThreshold) {
            if (path.empty()) return 0;

            const int pathSize = (int)path.size();
            if (startIndex < 0) startIndex = 0;
            if (startIndex >= pathSize) startIndex = 0;

            glm::vec3 posXZ = currentPosition;
            posXZ.y = 0.0f;

            glm::vec3 forwardXZ = NormalizeXZOr(currentForward, glm::vec3(0.0f, 0.0f, 1.0f));

            int bestIndex = -1;
            float bestDistSq = 0.0f;

            for (int step = 0; step < pathSize; step++) {
                int i = (startIndex + step) % pathSize;

                glm::vec3 p = path[i];
                p.y = 0.0f;

                glm::vec3 toPoint = p - posXZ;
                toPoint.y = 0.0f;

                float distSq = glm::dot(toPoint, toPoint);
                if (distSq < 0.000001f) {
                    return i;
                }

                float invLen = 1.0f / std::sqrt(distSq);
                glm::vec3 toDir = toPoint * invLen;

                float d = glm::dot(forwardXZ, toDir);
                if (d <= dotThreshold) continue;

                if (bestIndex == -1 || distSq < bestDistSq) {
                    bestIndex = i;
                    bestDistSq = distSq;
                }
            }

            if (bestIndex == -1) return startIndex;
            return bestIndex;
        }
    }

    void Shark::StraightenSpine(float deltaTime, float straightSpeed) {
        glm::vec3 fakeForwardMovement = GetForwardVector() * m_swimSpeed * deltaTime * straightSpeed;
        m_spine.Straighten(fakeForwardMovement);
    }

    void Shark::CalculateTargetFromPlayer() {
        if (m_huntedPlayerId != 0) {
            Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId);
            m_targetPosition = player->GetCameraPosition() - glm::vec3(0.0, 0.1f, 0.0f);

            static bool attackLeft = true;
            if (GetDistanceToTarget2D() < 6.5f) {
                if (attackLeft) {
                    m_targetPosition += m_left * 0.975f;
                }
                else {
                    m_targetPosition += m_right * 0.975f;
                }
            }
            else {
                attackLeft = !attackLeft;
            }
        }
    }

    void Shark::CalculateTargetFromPath() {
        if (m_path.empty()) return;

        const int pathSize = (int)m_path.size();
        if (m_nextPathPointIndex >= pathSize) m_nextPathPointIndex = 0;
        if (m_nextPathPointIndex < 0) m_nextPathPointIndex = 0;

        glm::vec3 headPos = GetHeadPosition2D();
        headPos.y = 0.0f;

        glm::vec3 forwardXZ = m_forward;
        forwardXZ.y = 0.0f;

        const float inFrontDotThreshold = 0.25f;

        m_nextPathPointIndex = GetClosestPointNotBehindSharkIndexForwardOnly(
            m_path,
            m_nextPathPointIndex,
            headPos,
            forwardXZ,
            inFrontDotThreshold
        );

        m_targetPosition = m_path[m_nextPathPointIndex];
        m_targetPosition.y = 0.0f;
    }

    void Shark::MoveShark(float deltaTime) {
        glm::vec3 displacement = m_forward * m_swimSpeed * deltaTime / (float)m_logicSubStepCount;
        m_spine.MoveLead(displacement);
    }

    void Shark::CalculateForwardVectorFromTarget(float deltaTime) {
        // Extract current orientations
        glm::vec3 headPos = GetHeadPosition2D();
        glm::vec3 targetPos = GetTargetPosition2D();

        // Compute desired forward
        glm::vec3 forwardXZ = NormalizeXZOr(m_forward, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 desiredXZ = NormalizeXZOr(targetPos - headPos, forwardXZ);

        // Find angle delta
        float dotValue = glm::clamp(glm::dot(forwardXZ, desiredXZ), -1.0f, 1.0f);
        float crossY = forwardXZ.x * desiredXZ.z - forwardXZ.z * desiredXZ.x;
        float signedAngle = std::atan2(crossY, dotValue);

        // Scale turn rate to ease in as shark aligns
        float baseTurnRate = glm::radians(225.0f);
        float alignmentScale = glm::clamp(std::abs(signedAngle) / glm::radians(45.0f), 0.1f, 1.0f);
        float currentTurnRate = baseTurnRate * alignmentScale;

        // Clamp angle delta
        float maxStep = currentTurnRate * deltaTime;
        if (signedAngle > maxStep) signedAngle = maxStep;
        if (signedAngle < -maxStep) signedAngle = -maxStep;

        // Build new forward vector
        float c = std::cos(signedAngle);
        float s = std::sin(signedAngle);

        glm::vec3 newForward;
        newForward.x = forwardXZ.x * c - forwardXZ.z * s;
        newForward.z = forwardXZ.x * s + forwardXZ.z * c;
        newForward.y = 0.0f;
        m_forward = NormalizeXZOr(newForward, forwardXZ);

        // Update orthogonal vectors
        const glm::vec3 up(0.0f, 1.0f, 0.0f);
        m_right = NormalizeXZOr(glm::cross(up, m_forward), glm::vec3(1.0f, 0.0f, 0.0f));
        m_left = -m_right;
    }

    void Shark::CalculateForwardVectorFromArrowKeys(float deltaTime) {
        float maxRotation = 5.0f;
        if (Input::KeyDown(HELL_KEY_LEFT)) {
            float blendFactor = glm::clamp(glm::abs(-maxRotation) / 90.0f, 0.0f, 1.0f);
            m_forward = glm::normalize(glm::mix(m_forward, m_left, blendFactor));
            std::cout << "PRESSED LEFT\n";
        }
        if (Input::KeyDown(HELL_KEY_RIGHT)) {
            float blendFactor = glm::clamp(glm::abs(maxRotation) / 90.0f, 0.0f, 1.0f);
            m_forward = glm::normalize(glm::mix(m_forward, m_right, blendFactor));
            std::cout << "PRESSED RIGHT\n";
        }
    }

    glm::vec3 Shark::GetForwardVector() {
        return m_forward;
    }

    glm::vec3 Shark::GetTargetPosition2D() {
        return m_targetPosition * glm::vec3(1.0f, 0.0f, 1.0f);
    }

    glm::vec3 Shark::GetHeadPosition2D() {
        return m_spine.GetLeadPosition() * glm::vec3(1.0f, 0.0f, 1.0f);
    }

    glm::vec3 Shark::GetCollisionLineEnd() {
        return GetCollisionSphereFrontPosition() + (GetForwardVector() * GetTurningRadius());
    }

    glm::vec3 Shark::GetCollisionSphereFrontPosition() {
        return GetHeadPosition2D() + GetForwardVector() * glm::vec3(kSharkCollisionSphereRadius);
    }

    float Shark::GetTurningRadius() const {
        float turningRadius = m_swimSpeed / m_rotationSpeed;
        return turningRadius;
    }

    bool Shark::TargetIsOnLeft(glm::vec3 targetPosition) {
        glm::vec3 lineStart = GetHeadPosition2D();
        glm::vec3 lineEnd = GetCollisionLineEnd();
        glm::vec3 lineNormal = Hell::LineMath::GetLineNormal(lineStart, lineEnd);
        glm::vec3 midPoint = Hell::LineMath::GetLineMidPoint(lineStart, lineEnd);
        return Hell::LineMath::IsPointOnOtherSideOfLine(lineStart, lineEnd, lineNormal, targetPosition);
    }

    float Shark::GetDistanceToTarget2D() {
        return glm::distance(GetHeadPosition2D() * glm::vec3(1, 0, 1), m_targetPosition * glm::vec3(1, 0, 1));
    }

    glm::vec3 Shark::GetMouthPosition3D() {
        Ragdoll* ragdoll = GetRagdoll();
        if (!ragdoll) return glm::vec3(0.0f);

        glm::mat4 headBoneTransform = ragdoll->GetRigidWorldTransform("BN_Head_00");
        return headBoneTransform[3];
    }

    glm::vec3 Shark::GetMouthPosition2D() {
        return GetMouthPosition3D() * glm::vec3(1.0f, 0.0f, 1.0f);
    }

    float Shark::GetDistanceMouthToTarget3D() {
        float fallback = 9999.0f;
        if (m_movementState == SharkMovementState::ARROW_KEYS ||
            m_movementState == SharkMovementState::STOPPED) {
            return fallback;
        }
        return glm::distance(GetMouthPosition3D(), m_targetPosition);
    }

    glm::vec3 Shark::GetEvadePoint3D() {
        return m_spine.GetLeadPosition() + (GetForwardVector() * glm::vec3(-0.0f));
    }

    glm::vec3 Shark::GetEvadePoint2D() {
        return GetEvadePoint3D() * glm::vec3(1.0f, 0.0f, 1.0f);
    }

    bool Shark::IsBehindEvadePoint(glm::vec3 position) {
        glm::vec3 position2D = position * glm::vec3(1.0f, 0.0f, 1.0f);
        glm::vec3 evadePoint2D = GetEvadePoint2D();

        glm::vec3 directionToPosition = position2D - evadePoint2D;
        if (glm::length(directionToPosition) < 1e-6f) {
            return false;
        }
        directionToPosition = glm::normalize(directionToPosition);

        glm::vec3 forwardVector = glm::normalize(GetForwardVector());

        float dotResult = glm::dot(directionToPosition, forwardVector);

        bool isBehind = dotResult < 0.0f;

        return isBehind;
    }
}
