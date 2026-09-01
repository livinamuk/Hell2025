#include "BulletSystem.h"

#include "Hell/Input.h"
#include "Hell/Common/Enum.h"
#include "Hell/Time.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Config/PhysicsConfig.h"

#include "Unloved/ObjectId.h"
#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/Systems/GameAudio/GameAudio.h"
#include "Unloved/World/World.h"

#include <iostream> // TODO: get me out of here
#include <unordered_map>
#include <unordered_set>

namespace Input = Hell::Input;

namespace Unloved::BulletSystem {
    bool g_awaitingFleshAudio = false;
    std::vector<Bullet> g_bullets;
    Hell::SlotMap<BulletTrail> g_bulletTrails;
    std::vector<BulletTrailParticle> g_bulletTrailParticles;

    struct HitGroup {
        std::unordered_set<uint64_t> objectIds;
        std::unordered_set<uint64_t> physicsIds;
        uint64_t lastTouchedUpdate = 0;
    };

    constexpr uint64_t HIT_GROUP_RETENTION_UPDATES = 120;
    uint64_t g_nextHitGroupId = 1;
    uint64_t g_updateCount = 0;
    std::unordered_map<uint64_t, HitGroup> g_hitGroups;

    void UpdateBulletTrails(float deltaTime);
    void UpdateBulletTrailParticles(float deltaTime);

    void ProcessDobermannHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessKangarooHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessPlayerHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessSharkHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessStandaloneRagdollHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ApplyRagdollImpact(uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition, bool wakeIfDisabled);

    bool RegisterHit(const Bullet& bullet, uint64_t objectId, uint64_t physicsId) {
        const uint64_t hitGroupId = bullet.GetHitGroupId();
        if (hitGroupId == 0) return true;

        HitGroup& hitGroup = g_hitGroups[hitGroupId];
        hitGroup.lastTouchedUpdate = g_updateCount;

        if (objectId != 0) {
            return hitGroup.objectIds.insert(objectId).second;
        }
        return hitGroup.physicsIds.insert(physicsId).second;
    }

    void RemoveExpiredHitGroups() {
        for (auto it = g_hitGroups.begin(); it != g_hitGroups.end();) {
            const bool expired = g_updateCount > it->second.lastTouchedUpdate + HIT_GROUP_RETENTION_UPDATES;
            if (expired) {
                it = g_hitGroups.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    uint64_t CreateHitGroup() {
        const uint64_t hitGroupId = g_nextHitGroupId++;
        if (g_nextHitGroupId == 0) g_nextHitGroupId = 1;
        return hitGroupId;
    }

    void AddBullet(BulletCreateInfo createInfo, uint64_t parentBulletTrailId) {
        g_bullets.push_back(Bullet(createInfo, parentBulletTrailId));
    }

    void AddBulletTrail(BulletCreateInfo createInfo) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::BULLET_TRAIL);
        g_bulletTrails.emplace_with_id(id, id, createInfo);
    }

    bool RemoveBulletTrail(uint64_t objectId) {
        if (!g_bulletTrails.contains(objectId)) {
            return false;
        }

        g_bulletTrails.get(objectId)->CleanUp();
        g_bulletTrails.erase(objectId);
        return true;
    }

    std::vector<Bullet>& GetBullets() {
        return g_bullets;
    }

    Hell::SlotMap<BulletTrail>& GetBulletTrails() {
        return g_bulletTrails;
    }

    std::vector<BulletTrailParticle>& GetBulletTrailParticles() {
        return g_bulletTrailParticles;
    }

    void Update() {
        ++g_updateCount;
        RemoveExpiredHitGroups();

        std::vector<Bullet>& bullets = GetBullets();
        std::vector<Bullet> newBullets;
        bool glassWasHit = false;
        bool rooDeath = false;

        for (Bullet& bullet : bullets) {
            // Cast PhysX ray
            glm::vec3 rayOrigin = bullet.GetOrigin();
            glm::vec3 rayDirection = bullet.GetDirection();
            float rayLength = bullet.GetRayLength();

            std::vector<PxRigidActor*> ignoredActors;

            int playerCount = Unloved::Session::GetLocalPlayerCount();
            for (int i = 0; i < playerCount; i++) {
                if (Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i)) {
                    if (PxRigidDynamic* characterControllerActor = player->GetCharacterControllerActor()) {
                        ignoredActors.push_back(characterControllerActor);
                    }
                }
            }

            Unloved::Player* player = Unloved::Session::GetPlayerById(bullet.GetOwnerObjectId());
            if (player) {
                auto RagdollActors = Hell::Physics::GetRagdollPxRigidActors(player->GetRagdollId());
                ignoredActors.insert(ignoredActors.end(), RagdollActors.begin(), RagdollActors.end());
            }

            PhysXRayResult physXRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, rayLength, false, ignoredActors);
            BvhRayResult bvhRayResult = Unloved::WorldBVH::ClosestHit(rayOrigin, bullet.GetDirection(), rayLength);

            // Defaults
            bool hitFound = false;
            uint64_t objectId = 0;
            uint64_t physicsId = 0;
            int32_t localMeshNodeIndex = -1;
            glm::vec3 hitPosition = glm::vec3(0.0f);
            glm::vec3 hitNormal = glm::vec3(0.0f);

            // BVH hit
            if (bvhRayResult.hitFound) {
                hitFound = true;
                objectId = bvhRayResult.objectId;
                physicsId = 0;
                localMeshNodeIndex = bvhRayResult.localMeshNodeIndex;
                hitPosition = bvhRayResult.hitPosition;
                hitNormal = bvhRayResult.hitNormal;

                //std::cout << "bvh hit found: " << bvhRayResult.hitPosition << "\n";
            }

            // PhysX hit
            if (physXRayResult.hitFound) {
                float distToPhysXHit = glm::distance(physXRayResult.hitPosition, bullet.GetOrigin());

                //std::cout << "physX hit found: " << physXRayResult.hitPosition << "\n";

                // Overwrite only if PhysX hit is closer
                if (!bvhRayResult.hitFound || bvhRayResult.hitFound && distToPhysXHit < bvhRayResult.distanceToHit) {
                    hitFound = true;
                    objectId = physXRayResult.userData.objectId;
                    physicsId = physXRayResult.userData.physicsId;
                    localMeshNodeIndex = -1;
                    hitPosition = physXRayResult.hitPosition;
                    hitNormal = physXRayResult.hitNormal;

                    //std::cout << "physX hit closer" << "\n";
                    //LegacyWorld::GetPictureFrames()[0].SetPosition(physXRayResult.hitPosition);
                   // LegacyWorld::GetPictureFrames()[0].SetScale(glm::vec3(0.0f));
                }
            }

            // Hit found?
            if (hitFound) {

                if (!RegisterHit(bullet, objectId, physicsId)) continue;

                ObjectType hitObjectType = Unloved::GetObjectIdType(objectId);
                Hell::Physics::PhysicsObjectType physicsObjectType = Hell::Physics::GetPhysicsObjectType(physicsId);


                glm::vec3 vatForward = glm::normalize(hitNormal * glm::vec3(1.0f, 0.0f, 1.0f));
                BloodSystem::SpawnVatBlood(hitPosition, vatForward, 0.05f, objectId);

                // std::cout << "\n";
                // std::cout << "Hit found " << Unloved::Session::GetSessionTime() << "\n";
                // std::cout << " ObjectId          " << objectId << "\n";
                // std::cout << " PhysicsId         " << physicsId << "\n";
                // std::cout << " ObjectType        " << Hell::Enum::ToString(hitObjectType) << "\n";
                // std::cout << " PhysicsObjectType " << Hell::Enum::ToString(physicsObjectType) << "\n";

                if (hitObjectType == ObjectType::DOBERMANN)          ProcessDobermannHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::KANGAROO)           ProcessKangarooHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::PLAYER)             ProcessPlayerHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::RAGDOLL_STANDALONE) ProcessStandaloneRagdollHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::SHARK)              ProcessSharkHit(objectId, physicsId, bullet, hitPosition);

                const glm::vec3 impactImpulse = bullet.GetDirection() * bullet.GetImpactImpulse();
                if (physicsObjectType == Hell::Physics::PhysicsObjectType::RIGID_DYNAMIC) {
                    Hell::Physics::AddImpulseAtPositionToRigidDynamic(physicsId, impactImpulse, hitPosition);
                }
                // This is probably sketchy...
                else if (hitObjectType == ObjectType::PICK_UP) {
                    if (PickUp* pickUp = Unloved::World::GetPickUpByObjectId(objectId)) {
                        pickUp->GetMeshNodes().AddImpulseAtPositionToPhysics(impactImpulse, hitPosition);
                    }
                }
                else if (hitObjectType == ObjectType::GENERIC_OBJECT) {
                    if (GenericObject* object = Unloved::World::GetGenericObjectById(objectId)) {
                        object->GetMeshNodes().AddImpulseAtPositionToPhysics(impactImpulse, hitPosition);
                    }
                }

                // Bullet enters water
                if (hitObjectType == ObjectType::WATER_PLANE_TOP) {
                    BulletCreateInfo createInfo = bullet.GetCreateInfo();
                    createInfo.origin = hitPosition + (bullet.GetDirection() * 0.1f);

                    AddBulletTrail(createInfo);
                    std::cout << "Spawning a new bullet trail beneath the ocean\n";
                }

                // Bullet leaves water
                if (hitObjectType == ObjectType::WATER_PLANE_BOTTOM) {
                    BulletCreateInfo bulletCreateInfo = bullet.GetCreateInfo();
                    bulletCreateInfo.origin = hitPosition + (bullet.GetDirection() * 0.1f);
                    bulletCreateInfo.direction = bullet.GetDirection();
                    bulletCreateInfo.rayLength = 1000.0f;
                    newBullets.emplace_back(Bullet(bulletCreateInfo));

                    std::cout << "Spawning a new bullet exiting the ocean\n";
                }

                // If this bullet belongs to a bullet trail, then destroy the trail
                if (bullet.GetParentBulletTrailId() != 0) {
                    RemoveBulletTrail(bullet.GetParentBulletTrailId());
                }

                // Retrieve the hit MeshNode, this could be nullptr if the hit was a physics object
                MeshNode* meshNode = World::GetMeshNodeByObjectIdAndLocalNodeIndex(objectId, localMeshNodeIndex);

                bool glassHit = meshNode && (meshNode->blendingMode == BlendingMode::GLASS ||
                                             meshNode->blendingMode == BlendingMode::MIRROR ||
                                             meshNode->blendingMode == BlendingMode::GLASS);

                bool createDecal = (meshNode && meshNode->decalType != DecalType::UNDEFINED) ||
                                   (Hell::Physics::GetRigidStaitcById(physicsId) != nullptr);

                if (!bullet.CreatesDecals()) {
                    createDecal = false;
                }

                // Create the decal
                if (createDecal) {
                    DecalCreateInfo decalCreateInfo;
                    decalCreateInfo.surfaceHitPosition = hitPosition;
                    decalCreateInfo.parentObjectId = objectId;
                    decalCreateInfo.localMeshNodeIndex = localMeshNodeIndex;
                    decalCreateInfo.surfaceHitNormal = hitNormal;
                    World::AddDecal(decalCreateInfo);

                    // Create second decal on opposite side if glass was hit + spawn a new bullet
                    if (glassHit) {
                        decalCreateInfo.surfaceHitNormal *= glm::vec3(-1.0f);
                        World::AddDecal(decalCreateInfo);

                        if (bullet.CreatesFolloWThroughBulletOnGlassHit() && meshNode->blendingMode != BlendingMode::MIRROR) {
                            BulletCreateInfo bulletCreateInfo = bullet.GetCreateInfo();
                            bulletCreateInfo.origin = hitPosition + bullet.GetDirection() * glm::vec3(0.05f);
                            bulletCreateInfo.direction = bullet.GetDirection();
                            newBullets.emplace_back(Bullet(bulletCreateInfo));
                        }

                        Unloved::GameAudio::PlayGlassHitAudio();
                    }
                }

                // Trigger the closest piano note on piano hit
                if (bullet.PlaysPiano()) {
                    if (Piano* piano = Unloved::World::GetPianoByObjectId(objectId)) {
                        piano->TriggerInternalNoteFromExternalBulletHit(hitPosition);
                    }
                }

                // Decal texture painting
                if (bullet.CreatesDecalTexturePaintedWounds()) {
                    DecalPaintingInfo decalPaintingInfo;
                    decalPaintingInfo.rayOrigin = bullet.GetOrigin();
                    decalPaintingInfo.rayDirection = bullet.GetDirection();
                    RenderDataManager::SubmitDecalPaintingInfo(decalPaintingInfo);
                }
            }
        }

        // Wipe old bullets, and replace with any new ones that got spawned from glass hits
        bullets = newBullets;

        float deltaTime = Hell::Time::DeltaTime();
        if (Input::KeyPressed(HELL_KEY_E)) {
            g_bulletTrails.clear();
            g_bulletTrailParticles.clear();
        }
        UpdateBulletTrails(deltaTime);
        UpdateBulletTrailParticles(deltaTime);
    }

    void UpdateBulletTrails(float deltaTime) {
        for (BulletTrail& bulletTrail : g_bulletTrails) {
            bulletTrail.Update(deltaTime);

            // Remove bullet if it traveled its max distance
            if (bulletTrail.m_distanceTraveled >= bulletTrail.m_maxDistance) {
                RemoveBulletTrail(bulletTrail.m_objectId);
            }
        }
    }

    void UpdateBulletTrailParticles(float deltaTime) {
        return;

        // THIS IS ALL DONE ON THE GPU NOIW
        // THIS IS ALL DONE ON THE GPU NOIW
        // THIS IS ALL DONE ON THE GPU NOIW
        // THIS IS ALL DONE ON THE GPU NOIW

        for (int i = 0; i < g_bulletTrailParticles.size();) {
            BulletTrailParticle& particle = g_bulletTrailParticles[i];

            // Water resistance
            particle.velocity *= 0.95f;
            particle.rotationalVelocity *= 0.98f;
            particle.lifeTime += deltaTime;

            // Remove if lifetime exceeds some value
            if (particle.lifeTime > 0.4f) {
                g_bulletTrailParticles.erase(g_bulletTrailParticles.begin() + i);
                continue;
            }
            else {
                i++;
            }

            // Step physics
            particle.position += particle.velocity * deltaTime;
            particle.rotation += particle.rotationalVelocity * deltaTime;
        }
    }

    void CleanUp() {
        for (BulletTrail& bulletTrail : g_bulletTrails) {
            bulletTrail.CleanUp();
        }

        g_bullets.clear();
        g_bulletTrails.clear();
        g_bulletTrailParticles.clear();
        g_hitGroups.clear();
        g_nextHitGroupId = 1;
        g_updateCount = 0;
    }

    // Ragdoll impact

    void ApplyRagdollImpact(uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition, bool wakeIfDisabled) {
        const Config::Physics::Settings& physicsSettings = Config::Physics::GetSettings();

        const glm::vec3 impactImpulse = bullet.GetDirection() * bullet.GetImpactImpulse();
        Hell::Physics::AddImpulseToRagdoll(physicsId, impactImpulse * physicsSettings.ragdollImpactTranslationScale, wakeIfDisabled);
        Hell::Physics::AddImpulseAtPositionToRagdoll(physicsId, impactImpulse * physicsSettings.ragdollImpactRotationScale, hitPosition, wakeIfDisabled);
    }

    // Dobermann hit

    void ProcessDobermannHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Dobermann* dobermann = Unloved::World::GetDobermannByObjectId(objectId);
        if (!dobermann) return;

        // Apply damage
        dobermann->TakeDamage(bullet.GetDamage());

        // Apply forces if dead
        if (dobermann->IsDead()) {
            ApplyRagdollImpact(physicsId, bullet, hitPosition, false);
        }

        BloodSystemOLD::AddBloodVAT(hitPosition, -bullet.GetDirection());
        GameAudio::TryPlayFleshImpactAudio();
    }

    // Kangaroo hit

    void ProcessKangarooHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Kangaroo* kangaroo = Unloved::World::GetKangarooByObjectId(objectId);
        if (!kangaroo) return;

        // Apply damage
        kangaroo->GiveDamage(bullet.GetDamage());

        // Apply forces if dead
        if (kangaroo->IsDead()) {
            ApplyRagdollImpact(physicsId, bullet, hitPosition, false);
        }

        BloodSystemOLD::AddBloodVAT(hitPosition, -bullet.GetDirection());
        GameAudio::TryPlayFleshImpactAudio();
    }

    // Player hit

    void ProcessPlayerHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Unloved::Player* player = Unloved::Session::GetPlayerById(objectId);
        if (!player) return;

        Ragdoll* ragdoll = player->GetRagdoll();
        if (!ragdoll) return;

        // Head shot
        if (ragdoll->GetBoneNameByPhysicsId(physicsId) == "CC_Base_Head") {
            player->Kill(true);
            std::cout << "[Player head shot]\n";
        }
        // body shot
        else {
            player->GiveDamage(bullet.GetDamage(), bullet.GetOwnerObjectId());
            std::cout << "[Player body shot]\n";
        }

        // Apply forces if dead
        if (player->IsDead()) {
            ApplyRagdollImpact(physicsId, bullet, hitPosition, false);
        }

        GameAudio::TryPlayFleshImpactAudio();
        BloodSystemOLD::AddBloodVAT(hitPosition, -bullet.GetDirection());
    }

    // Shark hit

    void ProcessSharkHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Shark* shark = Unloved::World::GetSharkByObjectId(objectId);
        if (!shark) return;

        // Apply damage
        shark->GiveDamage(bullet.GetOwnerObjectId(), bullet.GetDamage());

        // Apply forces if dead
        if (shark->IsDead()) {
            ApplyRagdollImpact(physicsId, bullet, hitPosition, false);
        }

        BloodSystemOLD::AddBloodVAT(hitPosition, -bullet.GetDirection());
        GameAudio::TryPlayFleshImpactAudio();
    }

    // Standalone Ragdoll hit

    void ProcessStandaloneRagdollHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        for (SkinnedGameObject& skinnedGameObject : World::GetSkinnedGameObjects()) {
            if (skinnedGameObject.GetOwnerObjectId() != objectId) continue;

            skinnedGameObject.SetAnimationModeToRagdoll();
            break;
        }

        GameAudio::TryPlayFleshImpactAudio();
        BloodSystemOLD::AddBloodVAT(hitPosition, bullet.GetDirection());

        ApplyRagdollImpact(physicsId, bullet, hitPosition, true);
    }
}
