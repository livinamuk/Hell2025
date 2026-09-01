#include "Player.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/LadderDismount.h"
#include "Unloved/Session/Session.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <limits>
#include <vector>

namespace Unloved {

namespace {
    constexpr float SOURCE_PLAYER_HEIGHT = 72.0f;
    constexpr float SOURCE_LADDER_SEARCH_DISTANCE = 64.0f;
    constexpr float SOURCE_LADDER_CONE_DISTANCE = 32.0f;
    constexpr float SOURCE_LADDER_ENDPOINT_DISTANCE = 16.0f;
    constexpr float SOURCE_LADDER_VERY_CLOSE_DISTANCE = 4.0f;
    constexpr float SOURCE_LADDER_DISMOUNT_ORIGIN_LIFT = 1.0f;
    constexpr float SOURCE_LADDER_DISMOUNT_SEARCH_RADIUS = 100.0f;
    constexpr float SOURCE_LADDER_DISMOUNT_RECHECK_DISTANCE = 40.0f;
    constexpr float SOURCE_LADDER_STRICT_DISMOUNT_DISTANCE = 40.0f;
    constexpr float SOURCE_MAX_CLIMB_SPEED = 200.0f;
    constexpr float SOURCE_LADDER_MAX_SEPARATION = 6.0f;
    constexpr float SOURCE_LADDER_HORIZONTAL_HEIGHT = 64.0f;
    constexpr float SOURCE_LADDER_ENDPOINT_TOLERANCE = 1.0f;
    constexpr float SOURCE_LADDER_JUMP_UP_SPEED = 50.0f;
    constexpr float LADDER_AUTO_MOUNT_ANGLE = 15.0f;
    constexpr float LADDER_AUTO_MOUNT_AXIS_DOT = 0.4f;
    constexpr float LADDER_MOVING_ALONG_AXIS_DOT = 0.9f;
    constexpr float LADDER_DISMOUNT_MINIMUM_VIEW_DOT = 0.5f;
    constexpr float LADDER_DISMOUNT_STRICT_VIEW_DOT = 0.7f;
    constexpr float MINIMUM_TRANSITION_DURATION = 0.001f;
    constexpr float MINIMUM_RAY_LENGTH = 0.0001f;

    glm::vec3 ClosestPointOnLineSegment(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
        const glm::vec3 segment = end - start;
        const float segmentLengthSquared = glm::dot(segment, segment);
        if (segmentLengthSquared <= 0.0f) {
            return start;
        }

        const float projection = glm::dot(point - start, segment) / segmentLengthSquared;
        const float clampedProjection = glm::clamp(projection, 0.0f, 1.0f);
        return start + segment * clampedProjection;
    }

    bool IsRayClear(const glm::vec3& start, const glm::vec3& end, const std::vector<physx::PxRigidActor*>& ignoredActors) {
        const glm::vec3 ray = end - start;
        const float rayLength = glm::length(ray);
        if (rayLength <= MINIMUM_RAY_LENGTH) {
            return true;
        }

        const glm::vec3 rayDirection = ray / rayLength;
        return !Hell::Physics::CastPhysXRay(start, rayDirection, rayLength, false, ignoredActors).hitFound;
    }

    glm::vec3 NormalizeOrZero(const glm::vec3& value) {
        const float length = glm::length(value);
        return length > 0.0f ? value / length : glm::vec3(0.0f);
    }

    float ParametricPositionOnLine(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
        const glm::vec3 segment = end - start;
        const float segmentLengthSquared = glm::dot(segment, segment);
        if (segmentLengthSquared <= 0.0f) {
            return 0.0f;
        }
        return glm::dot(point - start, segment) / segmentLengthSquared;
    }

    float DistanceSquaredToLine(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
        const float parametricPosition = ParametricPositionOnLine(point, start, end);
        const glm::vec3 projectedPoint = start + (end - start) * parametricPosition;
        const glm::vec3 separation = point - projectedPoint;
        return glm::dot(separation, separation);
    }

    bool PlayerCapsulesOverlap(
        const glm::vec3& firstFootPosition,
        float firstHeight,
        float firstRadius,
        const glm::vec3& secondFootPosition,
        float secondHeight,
        float secondRadius) {
        const float firstSegmentBottom = firstFootPosition.y + firstRadius;
        const float firstSegmentTop = firstFootPosition.y + firstHeight - firstRadius;
        const float secondSegmentBottom = secondFootPosition.y + secondRadius;
        const float secondSegmentTop = secondFootPosition.y + secondHeight - secondRadius;

        float verticalSeparation = 0.0f;
        if (firstSegmentTop < secondSegmentBottom) {
            verticalSeparation = secondSegmentBottom - firstSegmentTop;
        }
        else if (secondSegmentTop < firstSegmentBottom) {
            verticalSeparation = firstSegmentBottom - secondSegmentTop;
        }

        const float deltaX = firstFootPosition.x - secondFootPosition.x;
        const float deltaZ = firstFootPosition.z - secondFootPosition.z;
        const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ + verticalSeparation * verticalSeparation;
        const float combinedRadius = firstRadius + secondRadius;
        return distanceSquared < combinedRadius * combinedRadius;
    }

    struct NearbyLadderDismount {
        uint64_t objectId = 0;
        float distanceSquared = 0.0f;
    };

    std::vector<uint64_t> FindAssociatedLadderDismounts(
        const Ladder& ladder,
        float searchRadius,
        float recheckDistance) {
        std::vector<uint64_t> result;
        const float searchRadiusSquared = searchRadius * searchRadius;

        const auto addDismountsNearPosition = [&](const glm::vec3& position) {
            for (LadderDismount& dismount : World::GetLadderDismounts()) {
                const glm::vec3 difference = dismount.GetPosition() - position;
                if (glm::dot(difference, difference) >= searchRadiusSquared) {
                    continue;
                }
                if (std::find(result.begin(), result.end(), dismount.GetObjectId()) == result.end()) {
                    result.push_back(dismount.GetObjectId());
                }
            }
        };

        const glm::vec3 topPosition = ladder.GetTopPoint();
        glm::vec3 samplePosition = ladder.GetBottomPoint();
        const glm::vec3 ladderVector = topPosition - samplePosition;
        const float ladderLength = glm::length(ladderVector);
        const glm::vec3 ladderDirection = ladderLength > 0.0f ? ladderVector / ladderLength : glm::vec3(0.0f);

        addDismountsNearPosition(topPosition);
        addDismountsNearPosition(samplePosition);

        if (recheckDistance <= 0.0f) {
            return result;
        }

        float remainingLength = ladderLength;
        while (remainingLength > 0.0f) {
            remainingLength -= recheckDistance;
            if (remainingLength <= 0.0f) {
                break;
            }
            samplePosition += ladderDirection * recheckDistance;
            addDismountsNearPosition(samplePosition);
        }

        return result;
    }
}

float Player::ScaleSourceLadderDistance(float sourceDistance) const {
    return m_viewHeightStanding * (sourceDistance / SOURCE_PLAYER_HEIGHT);
}

float Player::GetLadderSearchDistance() const {
    return ScaleSourceLadderDistance(SOURCE_LADDER_SEARCH_DISTANCE);
}

Player::LadderCandidate Player::FindLadderCandidate(uint64_t skipLadderId) const {
    LadderCandidate result;
    float bestDistanceSquared = std::numeric_limits<float>::max();

    const glm::vec3 footPosition = GetFootPosition();
    const float searchDistance = GetLadderSearchDistance();
    const float searchDistanceSquared = searchDistance * searchDistance;

    std::vector<physx::PxRigidActor*> ignoredActors;
    if (physx::PxRigidActor* characterControllerActor = GetCharacterControllerActor()) {
        ignoredActors.push_back(characterControllerActor);
    }

    const std::vector<uint64_t>& ladderIds = World::GetLadders().ids();
    for (uint64_t ladderId : ladderIds) {
        if (ladderId == skipLadderId) {
            continue;
        }

        Ladder* ladder = World::GetLadderByObjectId(ladderId);
        if (!ladder) {
            continue;
        }

        const glm::vec3 closestPoint = ClosestPointOnLineSegment(footPosition, ladder->GetBottomPoint(), ladder->GetTopPoint());
        const float distanceSquared = glm::dot(closestPoint - footPosition, closestPoint - footPosition);
        if (distanceSquared > searchDistanceSquared) {
            continue;
        }

        bool visible = IsRayClear(footPosition, closestPoint, ignoredActors);
        if (!visible) {
            const glm::vec3 raisedFootPosition = footPosition + glm::vec3(0.0f, m_viewHeightStanding * 0.5f, 0.0f);
            visible = IsRayClear(raisedFootPosition, closestPoint, ignoredActors);
        }
        if (!visible) {
            continue;
        }

        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            result.ladderId = ladderId;
            result.closestPoint = closestPoint;
        }
    }

    return result;
}

bool Player::ShouldAutoMountLadderCone(const LadderCandidate& candidate) {
    const bool movingForward = PressingWalkForward() && !PressingWalkBackward();
    if (!candidate.ladderId || !movingForward) {
        return false;
    }

    Ladder* ladder = World::GetLadderByObjectId(candidate.ladderId);
    if (!ladder) {
        return false;
    }

    const glm::vec3 footPosition = GetFootPosition();
    const glm::vec3 ladderAxis = NormalizeOrZero(ladder->GetTopPoint() - ladder->GetBottomPoint());
    const glm::vec3 toLadder = candidate.closestPoint - footPosition;
    const float distanceToLadder = glm::length(toLadder);

    glm::vec3 flatToLadder = NormalizeOrZero(glm::vec3(toLadder.x, 0.0f, toLadder.z));
    glm::vec3 flatForward = GetCameraForward();
    flatForward.y = 0.0f;
    flatForward = NormalizeOrZero(flatForward);

    const float facingDot = glm::dot(flatForward, flatToLadder);
    const float facingAngle = std::acos(glm::clamp(facingDot, -1.0f, 1.0f)) * (180.0f / glm::pi<float>());
    const float coneDistance = ScaleSourceLadderDistance(SOURCE_LADDER_CONE_DISTANCE);
    const float veryCloseDistance = ScaleSourceLadderDistance(SOURCE_LADDER_VERY_CLOSE_DISTANCE);

    const bool closeToLadder = distanceToLadder != 0.0f && distanceToLadder < coneDistance;
    const bool veryCloseToLadder = distanceToLadder != 0.0f && distanceToLadder < veryCloseDistance;
    const bool facingLadder = facingAngle < LADDER_AUTO_MOUNT_ANGLE;
    const bool facingAlongAxis = std::abs(glm::dot(ladderAxis, GetCameraForward())) > LADDER_AUTO_MOUNT_AXIS_DOT;
    const bool strafing = PressingWalkLeft() != PressingWalkRight();

    return ((facingDot > 0.0f && !strafing) || facingAlongAxis) &&
           (facingLadder || veryCloseToLadder) &&
           closeToLadder;
}

bool Player::ShouldAutoMountLadderEndpoint(const LadderCandidate& candidate) {
    const bool movingForward = PressingWalkForward() && !PressingWalkBackward();
    if (!candidate.ladderId || !movingForward) {
        return false;
    }

    Ladder* ladder = World::GetLadderByObjectId(candidate.ladderId);
    if (!ladder) {
        return false;
    }

    const glm::vec3 footPosition = GetFootPosition();
    const float endpointDistance = ScaleSourceLadderDistance(SOURCE_LADDER_ENDPOINT_DISTANCE);
    const float endpointDistanceSquared = endpointDistance * endpointDistance;
    const float distanceToTopSquared = glm::dot(ladder->GetTopPoint() - footPosition, ladder->GetTopPoint() - footPosition);
    const float distanceToBottomSquared = glm::dot(ladder->GetBottomPoint() - footPosition, ladder->GetBottomPoint() - footPosition);

    if (distanceToTopSquared > endpointDistanceSquared && distanceToBottomSquared > endpointDistanceSquared) {
        return false;
    }

    glm::vec3 ladderAxis;
    if (distanceToTopSquared < endpointDistanceSquared) {
        ladderAxis = ladder->GetBottomPoint() - ladder->GetTopPoint();
    }
    else {
        ladderAxis = ladder->GetTopPoint() - ladder->GetBottomPoint();
    }

    ladderAxis = NormalizeOrZero(ladderAxis);
    return glm::dot(ladderAxis, GetCameraForward()) > LADDER_AUTO_MOUNT_AXIS_DOT;
}

bool Player::TryAutoMountLadder() {
    const LadderCandidate candidate = FindLadderCandidate();
    if (!candidate.ladderId) {
        return false;
    }

    if (ShouldAutoMountLadderCone(candidate)) {
        StartLadderTransition(true, candidate);
        return true;
    }

    if (ShouldAutoMountLadderEndpoint(candidate)) {
        StartLadderTransition(true, candidate);
        return true;
    }

    return false;
}

bool Player::IsLadderTransitionGoalReserved(const glm::vec3& goalPosition) const {
    for (int32_t playerIndex = 0; playerIndex < Session::GetLocalPlayerCount(); playerIndex++) {
        Player* otherPlayer = Session::GetLocalPlayerByViewportIndex(static_cast<uint32_t>(playerIndex));
        if (!otherPlayer || otherPlayer == this) {
            continue;
        }
        if (otherPlayer->m_movementMode != PlayerMovementMode::LADDER_TRANSITION ||
            !otherPlayer->m_ladderMoveData.destinationReserved) {
            continue;
        }

        if (PlayerCapsulesOverlap(
                goalPosition,
                m_viewHeightStanding,
                PLAYER_CAPSULE_RADIUS,
                otherPlayer->m_ladderMoveData.transitionGoalPosition,
                otherPlayer->m_viewHeightStanding,
                PLAYER_CAPSULE_RADIUS)) {
            return true;
        }
    }

    return false;
}

bool Player::IsLadderTransitionGoalClear(const glm::vec3& goalPosition, uint64_t ladderId) const {
    PxShape* controllerShape = GetCharacterControllerShape();
    PxRigidDynamic* controllerActor = GetCharacterControllerActor();
    PxScene* scene = Hell::Physics::GetPxScene();
    if (!controllerShape || !controllerActor || !scene) {
        return false;
    }

    const PxGeometryHolder controllerGeometry = controllerShape->getGeometry();
    const PxTransform currentShapePose = controllerActor->getGlobalPose() * controllerShape->getLocalPose();
    const glm::vec3 currentFootPosition = GetFootPosition();
    constexpr PxU32 OVERLAP_BUFFER_SIZE = 256;
    constexpr PxU32 PLAYER_COLLISION_MASK = static_cast<PxU32>(
        CollisionGroup::ENVIROMENT_OBSTACLE |
        CollisionGroup::ENVIROMENT_OBSTACLE_NO_DOG |
        CollisionGroup::CHARACTER_CONTROLLER);

    const auto positionIsClear = [&](const glm::vec3& testFootPosition) {
        PxTransform testShapePose = currentShapePose;
        const glm::vec3 translation = testFootPosition - currentFootPosition;
        testShapePose.p += PxVec3(translation.x, translation.y, translation.z);

        PxOverlapHit hitBuffer[OVERLAP_BUFFER_SIZE];
        PxOverlapBuffer overlapBuffer(hitBuffer, OVERLAP_BUFFER_SIZE);
        PxQueryFilterData queryFilterData;
        queryFilterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;

        if (!scene->overlap(controllerGeometry.any(), testShapePose, overlapBuffer, queryFilterData)) {
            return true;
        }

        for (PxU32 hitIndex = 0; hitIndex < overlapBuffer.getNbTouches(); hitIndex++) {
            const PxOverlapHit& hit = overlapBuffer.getTouch(hitIndex);
            if (!hit.actor || !hit.shape || hit.actor == controllerActor) {
                continue;
            }
            if (hit.shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE)) {
                continue;
            }

            const PhysicsUserData* userData = static_cast<const PhysicsUserData*>(hit.actor->userData);
            if (userData && userData->objectId == ladderId) {
                continue;
            }
            if (userData && userData->physicsType == PhysicsType::CHARACTER_CONTROLLER) {
                return false;
            }

            const PxFilterData hitFilterData = hit.shape->getQueryFilterData();
            if ((hitFilterData.word1 & PLAYER_COLLISION_MASK) != 0) {
                return false;
            }
        }

        return true;
    };

    return positionIsClear(goalPosition) && !IsLadderTransitionGoalReserved(goalPosition);
}

bool Player::StartLadderTransition(bool mounting, const LadderCandidate& candidate) {
    if (m_movementMode == PlayerMovementMode::LADDER_TRANSITION ||
        !candidate.ladderId ||
        !World::GetLadderByObjectId(candidate.ladderId) ||
        !IsLadderTransitionGoalClear(candidate.closestPoint, candidate.ladderId)) {
        return false;
    }

    PxShape* controllerShape = GetCharacterControllerShape();
    if (!controllerShape) {
        return false;
    }

    const glm::vec3 startPosition = GetFootPosition();
    const float transitionSpeed = m_walkingSpeed;
    const float transitionDistance = glm::distance(startPosition, candidate.closestPoint);

    LadderMoveData transitionData;
    transitionData.ladderId = candidate.ladderId;
    transitionData.dismountCandidateId = candidate.dismountId;
    transitionData.transitionStartPosition = startPosition;
    transitionData.transitionGoalPosition = candidate.closestPoint;
    transitionData.transitionDuration = std::max(transitionDistance / transitionSpeed, MINIMUM_TRANSITION_DURATION);
    transitionData.mounting = mounting;
    transitionData.destinationReserved = true;
    transitionData.startedThisFrame = true;
    if (!mounting) {
        transitionData.dismountCandidateDistance = m_ladderMoveData.dismountCandidateDistance;
        transitionData.dismountCandidateViewDot = m_ladderMoveData.dismountCandidateViewDot;
        transitionData.associatedDismountCount = m_ladderMoveData.associatedDismountCount;
        transitionData.nearbyDismountCount = m_ladderMoveData.nearbyDismountCount;
        transitionData.dismountStatus = LadderDismountStatus::STARTED;
    }

    const PxShapeFlags shapeFlags = controllerShape->getFlags();
    transitionData.simulationShapeWasEnabled = shapeFlags.isSet(PxShapeFlag::eSIMULATION_SHAPE);
    transitionData.sceneQueryShapeWasEnabled = shapeFlags.isSet(PxShapeFlag::eSCENE_QUERY_SHAPE);

    m_ladderMoveData = transitionData;
    controllerShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
    controllerShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
    m_movementMode = PlayerMovementMode::LADDER_TRANSITION;
    return true;
}

bool Player::TryExitLadderViaDismountNode(Ladder& ladder, bool strict) {
    m_ladderMoveData.dismountCandidateId = 0;
    m_ladderMoveData.dismountCandidateDistance = 0.0f;
    m_ladderMoveData.dismountCandidateViewDot = 0.0f;
    m_ladderMoveData.associatedDismountCount = 0;
    m_ladderMoveData.nearbyDismountCount = 0;

    const float searchRadius = ScaleSourceLadderDistance(SOURCE_LADDER_DISMOUNT_SEARCH_RADIUS);
    const float recheckDistance = ScaleSourceLadderDistance(SOURCE_LADDER_DISMOUNT_RECHECK_DISTANCE);
    const std::vector<uint64_t> associatedDismounts = FindAssociatedLadderDismounts(ladder, searchRadius, recheckDistance);
    m_ladderMoveData.associatedDismountCount = static_cast<int32_t>(associatedDismounts.size());
    if (associatedDismounts.empty()) {
        m_ladderMoveData.dismountStatus = LadderDismountStatus::NO_ASSOCIATED_POINTS;
        return false;
    }

    const glm::vec3 footPosition = GetFootPosition();
    const float searchRadiusSquared = searchRadius * searchRadius;
    std::vector<NearbyLadderDismount> nearbyDismounts;
    nearbyDismounts.reserve(associatedDismounts.size());

    for (uint64_t dismountId : associatedDismounts) {
        LadderDismount* dismount = World::GetLadderDismountByObjectId(dismountId);
        if (!dismount) {
            continue;
        }

        const glm::vec3 difference = dismount->GetPosition() - footPosition;
        const float distanceSquared = glm::dot(difference, difference);
        if (distanceSquared > searchRadiusSquared) {
            continue;
        }
        nearbyDismounts.push_back({ dismountId, distanceSquared });
    }

    std::sort(nearbyDismounts.begin(), nearbyDismounts.end(), [](const NearbyLadderDismount& lhs, const NearbyLadderDismount& rhs) {
        return lhs.distanceSquared < rhs.distanceSquared;
    });

    m_ladderMoveData.nearbyDismountCount = static_cast<int32_t>(nearbyDismounts.size());
    if (nearbyDismounts.empty()) {
        m_ladderMoveData.dismountStatus = LadderDismountStatus::NO_NEARBY_POINTS;
        return false;
    }

    bool foundClearDestination = false;
    bool foundFacingDestination = false;
    float bestViewDot = -std::numeric_limits<float>::max();
    float bestDistance = std::numeric_limits<float>::max();
    uint64_t bestDismountId = 0;
    glm::vec3 bestDestination = glm::vec3(0.0f);
    const float originLift = ScaleSourceLadderDistance(SOURCE_LADDER_DISMOUNT_ORIGIN_LIFT);

    for (const NearbyLadderDismount& nearbyDismount : nearbyDismounts) {
        LadderDismount* dismount = World::GetLadderDismountByObjectId(nearbyDismount.objectId);
        if (!dismount) {
            continue;
        }

        const glm::vec3 destination = dismount->GetPosition() + glm::vec3(0.0f, originLift, 0.0f);
        if (!IsLadderTransitionGoalClear(destination, ladder.GetObjectId())) {
            continue;
        }
        foundClearDestination = true;

        glm::vec3 directionToDestination = destination - GetCameraPosition();
        directionToDestination.y = 0.0f;
        const float distance = glm::length(directionToDestination);
        directionToDestination = NormalizeOrZero(directionToDestination);
        const float viewDot = glm::dot(directionToDestination, GetCameraForward());
        if (viewDot < LADDER_DISMOUNT_MINIMUM_VIEW_DOT) {
            continue;
        }
        foundFacingDestination = true;

        if (viewDot > bestViewDot) {
            bestViewDot = viewDot;
            bestDistance = distance;
            bestDismountId = dismount->GetObjectId();
            bestDestination = destination;
        }
    }

    if (!foundClearDestination) {
        m_ladderMoveData.dismountStatus = LadderDismountStatus::DESTINATION_BLOCKED_OR_RESERVED;
        return false;
    }
    if (!foundFacingDestination || !bestDismountId) {
        m_ladderMoveData.dismountStatus = LadderDismountStatus::NOT_FACING_POINT;
        return false;
    }

    m_ladderMoveData.dismountCandidateId = bestDismountId;
    m_ladderMoveData.dismountCandidateDistance = bestDistance;
    m_ladderMoveData.dismountCandidateViewDot = bestViewDot;

    const float strictDistance = ScaleSourceLadderDistance(SOURCE_LADDER_STRICT_DISMOUNT_DISTANCE);
    if (strict && (bestViewDot < LADDER_DISMOUNT_STRICT_VIEW_DOT || bestDistance > strictDistance)) {
        m_ladderMoveData.dismountStatus = LadderDismountStatus::STRICT_REJECTED;
        return false;
    }

    LadderCandidate candidate;
    candidate.ladderId = ladder.GetObjectId();
    candidate.dismountId = bestDismountId;
    candidate.closestPoint = bestDestination;
    if (!StartLadderTransition(false, candidate)) {
        m_ladderMoveData.dismountStatus = LadderDismountStatus::TRANSITION_REJECTED;
        return false;
    }

    return true;
}

void Player::LeaveLadder(const glm::vec3& velocity) {
    const glm::vec3 horizontalVelocity = glm::vec3(velocity.x, 0.0f, velocity.z);
    const float horizontalSpeed = glm::length(horizontalVelocity);

    m_ladderMoveData = {};
    m_movementMode = PlayerMovementMode::WALKING;
    m_yVelocity = velocity.y;
    m_grounded = false;
    m_groundedLastFrame = false;

    if (horizontalSpeed > 0.0f) {
        m_movementDirection = horizontalVelocity / horizontalSpeed;
        m_currentSpeed = horizontalSpeed;
        m_acceleration = 1.0f;
        m_moving = true;
    }
    else {
        m_movementDirection = glm::vec3(0.0f);
        m_currentSpeed = 0.0f;
        m_acceleration = 0.0f;
        m_moving = false;
    }
}

void Player::UpdateLadderMovement(float deltaTime) {
    Ladder* ladder = World::GetLadderByObjectId(m_ladderMoveData.ladderId);
    if (!ladder) {
        LeaveLadder(glm::vec3(0.0f));
        return;
    }

    const glm::vec3 bottomPosition = ladder->GetBottomPoint();
    const glm::vec3 topPosition = ladder->GetTopPoint();
    const glm::vec3 ladderVector = topPosition - bottomPosition;
    const float ladderLength = glm::length(ladderVector);
    if (ladderLength <= 0.0f) {
        LeaveLadder(glm::vec3(0.0f));
        return;
    }

    const glm::vec3 ladderUp = ladderVector / ladderLength;
    const glm::vec3 oldFootPosition = GetFootPosition();
    const float oldParametricPosition = ParametricPositionOnLine(oldFootPosition, bottomPosition, topPosition);
    const float maximumSeparation = ScaleSourceLadderDistance(SOURCE_LADDER_MAX_SEPARATION);

    if (DistanceSquaredToLine(oldFootPosition, bottomPosition, topPosition) > maximumSeparation * maximumSeparation) {
        LeaveLadder(m_ladderMoveData.ladderVelocity);
        return;
    }

    const float forwardInput = m_controlEnabled
        ? static_cast<float>(PressingWalkForward()) - static_cast<float>(PressingWalkBackward())
        : 0.0f;
    const float rightInput = m_controlEnabled
        ? static_cast<float>(PressingWalkRight()) - static_cast<float>(PressingWalkLeft())
        : 0.0f;
    const bool hasMovementInput = forwardInput != 0.0f || rightInput != 0.0f;
    const float climbSpeed = ScaleSourceLadderDistance(SOURCE_MAX_CLIMB_SPEED);

    m_grounded = false;
    m_groundedLastFrame = false;
    m_yVelocity = 0.0f;

    if (m_controlEnabled && PressingJump()) {
        glm::vec3 jumpDirection = NormalizeOrZero(GetCameraForward());
        if (forwardInput < 0.0f) {
            jumpDirection = -jumpDirection;
        }

        glm::vec3 jumpVelocity = jumpDirection * climbSpeed;
        if (GetCameraForward().y >= 0.0f) {
            jumpVelocity.y += ScaleSourceLadderDistance(SOURCE_LADDER_JUMP_UP_SPEED);
        }

        Hell::Physics::MoveCharacterController(m_characterControllerId, jumpVelocity * deltaTime);
        Hell::Physics::ClearCharacterControllerCollsionReports();
        LeaveLadder(jumpVelocity);
        return;
    }

    if (!hasMovementInput) {
        m_ladderMoveData.ladderVelocity = glm::vec3(0.0f);
        m_ladderMoveData.ladderParametricPosition = oldParametricPosition;
        m_ladderMoveData.ladderMoveDirection = 0.0f;
        m_ladderMoveData.dismountCandidateId = 0;
        m_ladderMoveData.dismountCandidateDistance = 0.0f;
        m_ladderMoveData.dismountCandidateViewDot = 0.0f;
        m_ladderMoveData.associatedDismountCount = 0;
        m_ladderMoveData.nearbyDismountCount = 0;
        m_ladderMoveData.dismountStatus = LadderDismountStatus::WAITING_FOR_ENDPOINT;
        m_movementDirection = glm::vec3(0.0f);
        m_acceleration = 0.0f;
        m_moving = false;
        return;
    }

    const glm::vec3 inputVelocity = NormalizeOrZero(
        GetCameraForward() * forwardInput + m_camera.GetRight() * rightInput);
    const bool nearlyHorizontal =
        std::abs(topPosition.y - bottomPosition.y) < ScaleSourceLadderDistance(SOURCE_LADDER_HORIZONTAL_HEIGHT);
    const float directionChangeover = nearlyHorizontal ? 0.0f : 0.3f;

    float moveDirection = 1.0f;
    if (inputVelocity.y >= 0.0f) {
        if (glm::dot(ladderUp, inputVelocity) < -directionChangeover) {
            moveDirection = -1.0f;
        }
    }
    else if (glm::dot(-ladderUp, inputVelocity) > directionChangeover) {
        moveDirection = -1.0f;
    }

    const glm::vec3 oldLadderVelocity = m_ladderMoveData.ladderVelocity;
    const glm::vec3 ladderVelocity = ladderUp * (climbSpeed * moveDirection);
    Hell::Physics::MoveCharacterController(m_characterControllerId, ladderVelocity * deltaTime);
    Hell::Physics::ClearCharacterControllerCollsionReports();

    const glm::vec3 newFootPosition = GetFootPosition();
    const float newParametricPosition = ParametricPositionOnLine(newFootPosition, bottomPosition, topPosition);
    const bool movingAlongLadder = std::abs(glm::dot(GetCameraForward(), ladderUp)) > LADDER_MOVING_ALONG_AXIS_DOT;
    const float distanceToTop = glm::distance(newFootPosition, topPosition);
    const float distanceToBottom = glm::distance(newFootPosition, bottomPosition);
    const float distanceToNearestEndpoint = std::min(distanceToTop, distanceToBottom);
    const bool nearDismountNode = distanceToNearestEndpoint < ScaleSourceLadderDistance(SOURCE_LADDER_ENDPOINT_DISTANCE);
    const bool nearDismountNodeThisFrame = distanceToNearestEndpoint < climbSpeed * deltaTime;
    const bool autoDismount = nearDismountNodeThisFrame && hasMovementInput && !movingAlongLadder;

    const float endpointTolerance = ScaleSourceLadderDistance(SOURCE_LADDER_ENDPOINT_TOLERANCE) / ladderLength;
    const bool leavingBelow = newParametricPosition < -endpointTolerance && newParametricPosition < oldParametricPosition;
    const bool leavingAbove = newParametricPosition > 1.0f + endpointTolerance && newParametricPosition > oldParametricPosition;
    const bool wouldLeaveLadder = leavingBelow || leavingAbove;

    m_ladderMoveData.ladderVelocity = ladderVelocity;
    m_ladderMoveData.ladderParametricPosition = newParametricPosition;
    m_ladderMoveData.ladderMoveDirection = moveDirection;
    m_ladderMoveData.dismountCandidateId = 0;
    m_ladderMoveData.dismountCandidateDistance = 0.0f;
    m_ladderMoveData.dismountCandidateViewDot = 0.0f;
    m_ladderMoveData.associatedDismountCount = 0;
    m_ladderMoveData.nearbyDismountCount = 0;
    m_ladderMoveData.dismountStatus = LadderDismountStatus::WAITING_FOR_ENDPOINT;
    m_movementDirection = ladderUp * moveDirection;
    m_acceleration = 1.0f;
    m_moving = true;

    if (!autoDismount) {
        if (wouldLeaveLadder) {
            SetFootPosition(oldFootPosition);
            m_ladderMoveData.ladderVelocity = oldLadderVelocity;
            m_ladderMoveData.ladderParametricPosition = oldParametricPosition;
        }
        return;
    }

    if (!wouldLeaveLadder && !nearDismountNode) {
        return;
    }

    if (TryExitLadderViaDismountNode(*ladder, false)) {
        m_ladderMoveData.startedThisFrame = false;
        return;
    }

    if (wouldLeaveLadder) {
        SetFootPosition(oldFootPosition);
        m_ladderMoveData.ladderVelocity = oldLadderVelocity;
        m_ladderMoveData.ladderParametricPosition = oldParametricPosition;
    }
}

void Player::UpdateLadderTransition(float deltaTime) {
    if (m_ladderMoveData.startedThisFrame) {
        m_ladderMoveData.startedThisFrame = false;
        return;
    }

    m_ladderMoveData.transitionElapsedTime += deltaTime;
    const bool transitionFinished = m_ladderMoveData.transitionElapsedTime > m_ladderMoveData.transitionDuration;
    const float transitionFraction = glm::clamp(
        m_ladderMoveData.transitionElapsedTime / m_ladderMoveData.transitionDuration,
        0.0f,
        1.0f);

    const glm::vec3 footPosition = glm::mix(
        m_ladderMoveData.transitionStartPosition,
        m_ladderMoveData.transitionGoalPosition,
        transitionFraction);
    SetFootPosition(footPosition);

    if (!transitionFinished) {
        return;
    }

    if (PxShape* controllerShape = GetCharacterControllerShape()) {
        controllerShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, m_ladderMoveData.simulationShapeWasEnabled);
        controllerShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, m_ladderMoveData.sceneQueryShapeWasEnabled);
    }

    const bool mounting = m_ladderMoveData.mounting;
    const uint64_t ladderId = m_ladderMoveData.ladderId;

    m_yVelocity = 0.0f;
    m_movementDirection = glm::vec3(0.0f);
    m_acceleration = 0.0f;
    m_moving = false;
    m_grounded = false;
    m_groundedLastFrame = false;

    m_ladderMoveData = {};
    if (mounting && World::GetLadderByObjectId(ladderId)) {
        m_ladderMoveData.ladderId = ladderId;
        m_movementMode = PlayerMovementMode::LADDER;
    }
    else {
        m_movementMode = PlayerMovementMode::WALKING;
    }
}

} // namespace Unloved
