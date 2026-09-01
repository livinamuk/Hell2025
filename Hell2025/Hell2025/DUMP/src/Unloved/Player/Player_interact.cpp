#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Logging.h"
#include "Hell/Input.h"

#include "Unloved/Render/Renderer.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <limits>

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

#pragma warning(disable : 26498)

namespace Unloved {

void Player::UpdateCursorRays() {
    m_physXRayResult.hitFound = false;
    m_bvhRayResult.hitFound = false;
    m_rayHitFound = false;

    if (!ViewportIsVisible()) return;

    float maxRayDistance = 1000.0f;

    // PhysX Ray
    glm::vec3 cameraRayOrigin = GetCameraPosition();
    glm::vec3 cameraRayDirection = GetCameraForward();

    std::vector<PxRigidActor*> ignoredActors;
    int playerCount = Unloved::Session::GetLocalPlayerCount();
    for (int i = 0; i < playerCount; i++) {
        if (Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i)) {
            if (PxRigidDynamic* characterControllerActor = player->GetCharacterControllerActor()) {
                ignoredActors.push_back(characterControllerActor);
            }

            auto ragdollActors = Hell::Physics::GetRagdollPxRigidActors(player->GetRagdollId());
            ignoredActors.insert(ignoredActors.end(), ragdollActors.begin(), ragdollActors.end());
        }
    }

    m_physXRayResult = Hell::Physics::CastPhysXRay(cameraRayOrigin, cameraRayDirection, maxRayDistance, false, ignoredActors);

    // Bvh Ray result
    glm::vec3 rayOrigin = GetCameraPosition();
    glm::vec3 rayDir = GetCameraForward();
    m_bvhRayResult = Unloved::WorldBVH::ClosestHit(rayOrigin, rayDir, maxRayDistance);
}


void Player::UpdateInteract() {
    m_interactObjectId = NO_ID;
    m_interactOpenableId = NO_ID;
    m_interactCustomId = NO_ID;

    if (!ViewportIsVisible()) return;

    // Probably make this cleaner, but for now this handles the fact you can interact while inventory is open.
    if (InventoryIsOpen()) return;

    m_interactHitPosition = glm::vec3(-9999.0f);
    m_interactHitNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    bool hitFound = false;

    PickUp* bvhPickUp = World::GetPickUpByObjectId(m_bvhRayResult.objectId);
    const bool bvhHitValid = m_bvhRayResult.hitFound && (!bvhPickUp || !bvhPickUp->IsDespawned());
    const bool bvhHitPickUp = bvhHitValid && bvhPickUp != nullptr;

    PickUp* physXPickUp = World::GetPickUpByObjectId(m_physXRayResult.userData.objectId);
    const bool physXHitValid = m_physXRayResult.hitFound && (!physXPickUp || !physXPickUp->IsDespawned());

    // Replace me with some distance check with closest point from hit object AABB
    if (bvhHitValid) {
        m_interactObjectId = m_bvhRayResult.objectId;
        m_interactOpenableId = m_bvhRayResult.openableId;
        m_interactCustomId = m_bvhRayResult.customId;
        m_interactHitPosition = m_bvhRayResult.hitPosition;
        m_interactHitNormal = m_bvhRayResult.hitNormal;
        hitFound = true;
    }

    // A visible pickup under the crosshair wins
    if (physXHitValid && !bvhHitPickUp && (!bvhHitValid || m_physXRayResult.distanceToHit < m_bvhRayResult.distanceToHit)) {
        m_interactObjectId = m_physXRayResult.userData.objectId;
        m_interactOpenableId = 0;
        m_interactCustomId = 0;
        m_interactHitPosition = m_physXRayResult.hitPosition;
        m_interactHitNormal = m_physXRayResult.hitNormal;
        hitFound = true;
    }

    PickUp* directPickUp = World::GetPickUpByObjectId(m_interactObjectId);

    // Sweep test
    if (hitFound && !directPickUp) {
        float sphereRadius = 0.15f;
        glm::vec3 spherePosition = m_interactHitPosition - GetCameraForward() * (sphereRadius * 1.25f);

        PxCapsuleGeometry overlapSphereShape = PxCapsuleGeometry(sphereRadius, 0);
        const PxTransform overlapSphereTranform = PxTransform(Hell::Physics::GlmVec3toPxVec3(spherePosition));
        PhysXOverlapReport overlapReport = Hell::Physics::OverlapTest(overlapSphereShape, overlapSphereTranform, CollisionGroup(GENERIC_BOUNCEABLE | GENERTIC_INTERACTBLE | ITEM_PICK_UP | ENVIROMENT_OBSTACLE));

        auto distanceToPickUp = [this](const PhysXOverlapResult& hit) {
            PickUp* pickUp = World::GetPickUpByObjectId(hit.userData.objectId);
            if (!pickUp || pickUp->IsDespawned()) return std::numeric_limits<float>::max();

            float closestDistance = std::numeric_limits<float>::max();
            for (const MeshNode& meshNode : pickUp->GetMeshNodes().GetNodes()) {
                if (meshNode.blendingMode == BlendingMode::DO_NOT_RENDER) continue;

                const glm::vec3 closestPoint = meshNode.worldSpaceObb.ClosestPoint(m_interactHitPosition);
                const glm::vec3 delta = closestPoint - m_interactHitPosition;
                closestDistance = std::min(closestDistance, glm::dot(delta, delta));
            }
            return closestDistance;
        };

        // Pick whatever is visually closest to the crosshair
        sort(overlapReport.hits.begin(), overlapReport.hits.end(), [&distanceToPickUp](const PhysXOverlapResult& lhs, const PhysXOverlapResult& rhs) {
            return distanceToPickUp(lhs) < distanceToPickUp(rhs);
        });

        for (const PhysXOverlapResult& hit : overlapReport.hits) {
            PickUp* pickUp = World::GetPickUpByObjectId(hit.userData.objectId);
            if (!pickUp || pickUp->IsDespawned()) continue;

            m_interactObjectId = hit.userData.objectId;
            m_interactOpenableId = 0;
            m_interactCustomId = 0;
            break;
        }
    }

    //DebugDraw::DrawPoint(hitPosition, GREEN);

    ObjectType interactObjectType = Unloved::GetObjectIdType(m_interactObjectId);

    // Convenience bool for setting crosshair
    m_interactFound = false;

    if (Unloved::OpenableManager::IsInteractable(m_interactOpenableId, GetCameraPosition())) m_interactFound = true;
    if (interactObjectType == ObjectType::PIANO && m_interactCustomId != 0) {
        m_interactFound = true;
    }
    if (PickUp* pickUp = World::GetPickUpByObjectId(m_interactObjectId); pickUp && !pickUp->IsDespawned()) m_interactFound = true;

    // Bail if nothing to interact with
    if (!InteractFound()) return;

    // PRESSED interact key
    if (PressedInteract()) {
        if (Unloved::OpenableManager::GetOpenableByOpenableId(m_interactOpenableId)) {

            std::string openableText = Unloved::OpenableManager::TriggerInteract(m_interactOpenableId, GetCameraPosition(), GetCameraForward());
            if (openableText != "") {
                m_typeWriter.DisplayText(openableText);
            }
        }

        // Pickups
        if (PickUp* pickUp = Unloved::World::GetPickUpByObjectId(m_interactObjectId)) {

            if (!pickUp->IsDespawned()) {

                if (pickUp->GetType() == ItemType::WEAPON) {
                    m_inventory.GiveWeapon(pickUp->GetName());
                }
                else if (pickUp->GetType() == ItemType::AMMO) {
                    m_inventory.GiveAmmo(pickUp->GetName(), Bible::GetAmmoPickUpAmount(pickUp->GetName()));
                }
                else if (pickUp->GetType() == ItemType::UNDEFINED) {
                    Logging::Warning() << "Player " << m_viewportIndex << " tried to pick up a PickUp with name '" << pickUp->GetName() << "' but type '" << Hell::Enum::ToString(pickUp->GetType()) << "'";
                }
                else if (pickUp->GetType() == ItemType::HEAL) {
                    m_inventory.AddInventoryItem(pickUp->GetName());
                }
                else {
                    Logging::Error() << "You picked up a Pickup of type " << Hell::Enum::ToString(pickUp->GetType()) << " which you haven't written a code path for within Player::UpdateInteract()\n";
                }

                if (pickUp->GetCreateInfo().respawn) {
                    pickUp->Despawn();
                    for (Light& light : Unloved::World::GetLights()) {
                        light.ForceDirty();
                    }
                }
                else {
                    World::RemoveObjectById(m_interactObjectId);
                }
                Audio::PlayAudio("ItemPickUp.wav", 1.0f);
            }
        }
    }

    // PRESSING interact key
    if (PressingInteract()) {

        // Piano keys
        if (interactObjectType == ObjectType::PIANO && m_interactCustomId != 0) {
            if (Piano* piano = Unloved::World::GetPianoByObjectId(m_interactObjectId)) {
                piano->PressKey(m_interactCustomId);
            }
        }
    }
}

}
