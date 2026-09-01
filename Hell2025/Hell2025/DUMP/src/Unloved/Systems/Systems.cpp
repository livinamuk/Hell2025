#include "Systems.h"

#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/Time.h"

#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/Systems/FeatureTest/FeatureTest.h"
#include "Unloved/Systems/GameAudio/GameAudio.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Systems/PianoPlayback/PianoPlaybackManager.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"

namespace Unloved::Systems {
    namespace {
        void UpdateOpenables(float deltaTime) {
            ProfilerCPUZone("Openables");
            OpenableManager::Update(deltaTime);
        }

        void UpdateNavMesh() {
            ProfilerCPUZone("Nav mesh");
            NavMeshManager::Update();
        }

        void UpdatePianoPlayback() {
            ProfilerCPUZone("Piano playback");
            PianoPlaybackManager::Update();
        }

        void UpdateGameAudio() {
            ProfilerCPUZone("Game audio");
            GameAudio::Update();
        }

        void UpdateFeatureTest() {
            ProfilerCPUZone("Feature test");
            FeatureTest::Update();
        }

        void UpdateOldBloodSystem(float deltaTime) {
            ProfilerCPUZone("Old blood system");
            BloodSystemOLD::Update(deltaTime);
        }
    }

    void Init() {
        NavMeshManager::Init();
        PianoPlaybackManager::Init();
    }

    void BeginFrame() {
        BloodSystem::BeginFrame();
        DirtyTracker::BeginFrame();
        GameAudio::BeginFrame();
        ShadowMapManager::BeginFrame();
    }

    void PreWorldUpdate() {
        ProfilerCPUZoneFunction();

        const float deltaTime = Hell::Time::DeltaTime();

        Animator::Update(deltaTime);
        UpdateOpenables(deltaTime);
        UpdateNavMesh();
        UpdatePianoPlayback();
        UpdateGameAudio();
        UpdateFeatureTest();
        UpdateOldBloodSystem(deltaTime);
    }

    void PostWorldUpdate() {
        BloodSystem::Update();
        DirtyTracker::Update();
        MirrorManager::Update();
        ShadowMapManager::Update();
    }

    void CleanUp() {
        Animator::CleanUp();
        BloodSystemOLD::CleanUp();
        BulletSystem::CleanUp();
        PianoPlaybackManager::CleanUp();
    }
}
