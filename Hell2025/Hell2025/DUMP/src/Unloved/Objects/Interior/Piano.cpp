#include "Piano.h"

#include "Hell/Audio.h"
#include "Hell/Audio/Synth.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/ObjectId.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Legacy/World/LegacyWorld.h"

namespace Unloved {

Piano::Piano(uint64_t id, PianoCreateInfo createInfo) {
    Init(id, createInfo, SpawnOffset());
}

void Piano::Init(uint64_t id, PianoCreateInfo createInfo) {
    Init(id, createInfo, SpawnOffset());
}

Piano::Piano(uint64_t id, PianoCreateInfo createInfo, SpawnOffset spawnOffset) {
    Init(id, createInfo, spawnOffset);
}

void Piano::Init(uint64_t id, PianoCreateInfo createInfo, SpawnOffset spawnOffset) {
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    m_soundFontName = m_createInfo.soundFontName;

    m_transform.position = m_createInfo.position;
    m_transform.rotation = m_createInfo.rotation;

    m_pianoObjectId = id;

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& topCover = meshNodeCreateInfoSet.emplace_back();
    topCover.openable.isOpenable = false;                                            // YOU MADE THIS FALSE COZ OF THE TOKAREV ON TOP
    topCover.materialName = "Piano0";
    topCover.meshName = "Yamaha_Case.Top.Cover";
    topCover.openable.openAxis = OpenAxis::ROTATE_X_NEG;
    topCover.openable.initialOpenState = OpenState::CLOSED;
    topCover.openable.minOpenValue = 0.0f;
    topCover.openable.maxOpenValue = HELL_PI;
    topCover.openable.openSpeed = 8.5f;
    topCover.openable.closeSpeed = 8.5f;

    MeshNodeCreateInfo& keyboardCover = meshNodeCreateInfoSet.emplace_back();
    keyboardCover.openable.isOpenable = true;
    keyboardCover.materialName = "Piano0";
    keyboardCover.meshName = "Yamaha_Keyboard.Cover";
    keyboardCover.openable.additionalTriggerMeshNames = { "Yamaha_Keyboard.Cover.Lock" };
    keyboardCover.openable.openAxis = OpenAxis::ROTATE_X_NEG;
    keyboardCover.openable.initialOpenState = OpenState::OPEN;
    keyboardCover.openable.minOpenValue = 0.0f;
    keyboardCover.openable.maxOpenValue = 2.1f;
    keyboardCover.openable.openSpeed = 8.5f;
    keyboardCover.openable.closeSpeed = 8.5f;

    MeshNodeCreateInfo& sheetMusicRest = meshNodeCreateInfoSet.emplace_back();
    sheetMusicRest.openable.isOpenable = true;
    sheetMusicRest.materialName = "Piano0";
    sheetMusicRest.meshName = "Yamaha_Lyrics.Stand";
    sheetMusicRest.openable.additionalTriggerMeshNames = { "Yamaha_Lyrics.Hinges.Plate.A",  "Yamaha_Lyrics.Hinges.Plate.B" };
    sheetMusicRest.openable.openAxis = OpenAxis::ROTATE_X_NEG;
    sheetMusicRest.openable.initialOpenState = OpenState::CLOSED;
    sheetMusicRest.openable.minOpenValue = 0.0f;
    sheetMusicRest.openable.maxOpenValue = HELL_PI * 0.7f;
    sheetMusicRest.openable.openSpeed = 8.5f;
    sheetMusicRest.openable.closeSpeed = 8.5f;

    MeshNodeCreateInfo& sheetMusicRestHingesA = meshNodeCreateInfoSet.emplace_back();
    sheetMusicRestHingesA.meshName = "Yamaha_Lyrics.Stand.Hinges.Plate.A";
    sheetMusicRestHingesA.materialName = "Piano1";

    MeshNodeCreateInfo& sheetMusicRestHingesB = meshNodeCreateInfoSet.emplace_back();
    sheetMusicRestHingesB.meshName = "Yamaha_Lyrics.Stand.Hinges.Plate.B";
    sheetMusicRestHingesB.materialName = "Piano1";

    MeshNodeCreateInfo& topCoverLock = meshNodeCreateInfoSet.emplace_back();
    topCoverLock.meshName = "Yamaha_Keyboard.Cover.Lock";
    topCoverLock.materialName = "Piano0";

    MeshNodeCreateInfo& body = meshNodeCreateInfoSet.emplace_back();
    body.meshName = "Yamaha_Main";
    body.materialName = "Piano0";

    MeshNodeCreateInfo& topHingePlate = meshNodeCreateInfoSet.emplace_back();
    topHingePlate.meshName = "Yamaha_MusicBox.Hinge.Plate";
    topHingePlate.materialName = "Piano1";

    MeshNodeCreateInfo& topHinge = meshNodeCreateInfoSet.emplace_back();
    topHinge.meshName = "Yamaha_MusicBox.Cover.Hinge";
    topHinge.materialName = "Piano1";

    MeshNodeCreateInfo& hingeStick = meshNodeCreateInfoSet.emplace_back();
    hingeStick.meshName = "Yamaha_Keyboard.Cover.Hinge.Stick";
    hingeStick.materialName = "Piano1";

    MeshNodeCreateInfo& coverHingePlate = meshNodeCreateInfoSet.emplace_back();
    coverHingePlate.meshName = "Yamaha_Keyboard.Cover.Hinge.Plate";
    coverHingePlate.materialName = "Piano1";

    MeshNodeCreateInfo& coverLock = meshNodeCreateInfoSet.emplace_back();
    coverLock.meshName = "Yamaha_Keyboard.Cover.Lock";
    coverLock.materialName = "Piano1";

    MeshNodeCreateInfo& keyBedLock = meshNodeCreateInfoSet.emplace_back();
    keyBedLock.meshName = "Yamaha_KeyBed.Lock";
    keyBedLock.materialName = "Piano1";

    MeshNodeCreateInfo& pedalL = meshNodeCreateInfoSet.emplace_back();
    pedalL.meshName = "Yamaha_Soft.&.Damper.Pedal.L";
    pedalL.materialName = "Piano1";

    MeshNodeCreateInfo& pedalR = meshNodeCreateInfoSet.emplace_back();
    pedalR.meshName = "Yamaha_Soft.&.Damper.Pedal.R";
    pedalR.materialName = "Piano1";

    MeshNodeCreateInfo& pedalM = meshNodeCreateInfoSet.emplace_back();
    pedalM.meshName = "Yamaha_Muffler.Pedal";
    pedalM.materialName = "Piano1";

    MeshNodeCreateInfo& hingePin = meshNodeCreateInfoSet.emplace_back();
    hingePin.meshName = "Yamaha_HingePin.001";
    hingePin.materialName = "Piano1";

    if (Model* model = Hell::ResourceManager::GetModelByName("Piano")) {
        for (uint32_t meshId : model->GetMeshIndices()) {
            if (Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId)) {
                if (mesh->name.find("Yamaha_Key_") != std::string::npos) {
                    uint64_t keyId = Unloved::GetNextCustomId();
                    PianoKey& pianoKey = m_keys[keyId];
                    pianoKey.m_meshName = mesh->name;
                    pianoKey.m_note = MeshNameToNote(mesh->name);
                    pianoKey.m_isSharp = (mesh->name.find("#") != std::string::npos);

                    MeshNodeCreateInfo& meshNodeCreateInfo = meshNodeCreateInfoSet.emplace_back();
                    meshNodeCreateInfo.meshName = mesh->name;
                    meshNodeCreateInfo.materialName = "Piano1";
                    meshNodeCreateInfo.customId = keyId;
                    meshNodeCreateInfo.forceDynamic = true;
                }
            }
        }
    }

    m_meshNodes.Init(m_pianoObjectId, "Piano", meshNodeCreateInfoSet);

    m_seatPosition = m_transform.to_mat4() * glm::vec4(0.75f, 0.0f, 0.75f, 1.0f);

    // Create physics objects
    //PhysicsFilterData filterData;
    //filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    //filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    //filterData.collidesWith = (CollisionGroup)(CHARACTER_CONTROLLER | BULLET_CASING | ITEM_PICK_UP | RAGDOLL_ENEMY);
    //m_rigidStaticId = Hell::Physics::CreateRigidStaticConvexMeshFromModel(m_transform, "PianoConvexMesh", filterData);
    //
    //PhysicsUserData userData;
    //userData.physicsId = m_rigidStaticId;
    //userData.objectId = m_pianoObjectId;
    //userData.physicsType = PhysicsType::RIGID_STATIC;
    //userData.objectType = ObjectType::PIANO;
    //Hell::Physics::SetRigidStaticUserData(m_rigidStaticId, userData);

    CalculatePianoKeyWorldspaceCenters();
}

void Piano::PressKey(uint32_t customId) {
    auto it = m_keys.find(customId);
    if (it != m_keys.end()) {
        PianoKey& key = it->second;
        key.PressKey(m_soundFontName);
    }
}

void Piano::CalculatePianoKeyWorldspaceCenters() {
    m_meshNodes.Update(m_transform.to_mat4());

    for (auto& pair : m_keys) {
        PianoKey& key = pair.second;

        if (const AABB* aabb = m_meshNodes.GetWorldSpaceAabbByMeshName(key.m_meshName)) {
            key.m_worldSpaceCenter = aabb->GetCenter();
        }
    }
}

void Piano::SetPosition(glm::vec3 position) {
    m_createInfo.position = position;
    m_transform.position = position;

    // Update collision mesh position
    Hell::Physics::SetRigidStaticWorldTransform(m_rigidStaticId, m_transform.to_mat4());

    CalculatePianoKeyWorldspaceCenters();
    NavMeshManager::MarkStaticDirty();
}

void Piano::SetRotation(glm::vec3 rotation) {
    m_createInfo.rotation = rotation;
    m_transform.rotation = rotation;

    // Update collision mesh rotation
    Hell::Physics::SetRigidStaticWorldTransform(m_rigidStaticId, m_transform.to_mat4());

    CalculatePianoKeyWorldspaceCenters();
    NavMeshManager::MarkStaticDirty();
}

void Piano::CleanUp() {
    Hell::Physics::MarkRigidStaticForRemoval(m_rigidStaticId);
	m_meshNodes.CleanUp();
}

void Piano::Update(float deltaTime) {
    m_worldMatrix = m_transform.to_mat4();

    // Update keys
    for (auto& pair : m_keys) {
        const uint32_t& objectId = pair.first;

        PianoKey& key = pair.second;
        key.Update(deltaTime, m_soundFontName);

        if (key.IsDirty()) {
            Transform transform;
            transform.position.y = key.m_yTranslation;
            transform.rotation.x = key.m_xRotation;

            m_meshNodes.SetTransformByMeshName(key.m_meshName, transform);
            m_meshNodes.ForceDirty();
        }
    }

    m_meshNodes.Update(m_transform.to_mat4());
}

void Piano::TriggerInternalNoteFromExternalBulletHit(glm::vec3 bulletHitPositon) {
    float closetDistance = std::numeric_limits<float>::max();
    PianoKey* closetKey = nullptr;

    for (auto& pair : m_keys) {
        const uint64_t& objectId = pair.first;
        PianoKey& key = pair.second;
        float distance = glm::distance(key.m_worldSpaceCenter, bulletHitPositon);
        if (distance < closetDistance) {
            closetDistance = distance;
            closetKey = &key;
        }
    }

    if (closetKey) {
        closetKey->PressKey(m_soundFontName, 127, 0.1f);
    }
}

bool Piano::PianoKeyExists(uint64_t pianoKeyId) {
    return (m_keys.find(pianoKeyId) != m_keys.end());
}

void Piano::PlayMajorFirstInversion(int rootNote) {

    int fifth = rootNote - 5;
    int majorThird = rootNote + 4;

    for (auto& pair : m_keys) {
        const uint64_t& objectId = pair.first;
        PianoKey& key = pair.second;

        if (key.m_note == rootNote) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == majorThird) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == fifth) {
            key.PressKey(m_soundFontName);
        }
    }
}

void Piano::PlayMajor7th(int rootNote) {

    int seventh = rootNote - 2;
    int majorThird = rootNote - 5 - 3;

    for (auto& pair : m_keys) {
        const uint64_t& objectId = pair.first;
        PianoKey& key = pair.second;

        if (key.m_note == rootNote) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == seventh) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == majorThird) {
            key.PressKey(m_soundFontName);
        }
    }
}

void Piano::PlayMinor(int rootNote) {
    int minorThird = rootNote + 3;
    int fifth = rootNote + 7;

    for (auto& pair : m_keys) {
        PianoKey& key = pair.second;

        if (key.m_note == rootNote) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == minorThird) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == fifth) {
            key.PressKey(m_soundFontName);
        }
    }
}


void Piano::PlayMajor(int rootNote) {
    int third = rootNote + 4;
    int fifth = rootNote + 7;

    for (auto& pair : m_keys) {
        PianoKey& key = pair.second;

        if (key.m_note == rootNote) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == third) {
            key.PressKey(m_soundFontName);
        }
        if (key.m_note == fifth) {
            key.PressKey(m_soundFontName);
        }
    }
}

void Piano::PlayKey(int note, int velocity, float duration) {
    for (auto& pair : m_keys) {
        PianoKey& key = pair.second;
        if (key.m_note == note) {
            key.PressKey(m_soundFontName, velocity, duration);
        }
    }
}

void Piano::SetSustain(bool value) {
    if (Hell::Synth::SelectSoundFont(m_soundFontName)) {
        Hell::Synth::SetSustain(value);
    }
}


PianoKey* Piano::GetPianoKey(uint64_t pianoKeyId) {
    if (!PianoKeyExists(pianoKeyId)) return nullptr;

    return &m_keys[pianoKeyId];
}

uint32_t Piano::MeshNameToNote(const std::string& meshName) {
    std::vector<std::string> meshNames(88);

    meshNames[0] = "Yamaha_Key_A0";
    meshNames[1] = "Yamaha_Key_A0#";
    meshNames[2] = "Yamaha_Key_B0";
    meshNames[3] = "Yamaha_Key_C1";
    meshNames[4] = "Yamaha_Key_C1#";
    meshNames[5] = "Yamaha_Key_D1";
    meshNames[6] = "Yamaha_Key_D1#";
    meshNames[7] = "Yamaha_Key_E1";
    meshNames[8] = "Yamaha_Key_F1";
    meshNames[9] = "Yamaha_Key_F1#";
    meshNames[10] = "Yamaha_Key_G1";
    meshNames[11] = "Yamaha_Key_G1#";
    meshNames[12] = "Yamaha_Key_A1";
    meshNames[13] = "Yamaha_Key_A1#";
    meshNames[14] = "Yamaha_Key_B1";
    meshNames[15] = "Yamaha_Key_C2";
    meshNames[16] = "Yamaha_Key_C2#";
    meshNames[17] = "Yamaha_Key_D2";
    meshNames[18] = "Yamaha_Key_D2#";
    meshNames[19] = "Yamaha_Key_E2";
    meshNames[20] = "Yamaha_Key_F2";
    meshNames[21] = "Yamaha_Key_F2#";
    meshNames[22] = "Yamaha_Key_G2";
    meshNames[23] = "Yamaha_Key_G2#";
    meshNames[24] = "Yamaha_Key_A2";
    meshNames[25] = "Yamaha_Key_A2#";
    meshNames[26] = "Yamaha_Key_B2";
    meshNames[27] = "Yamaha_Key_C3";
    meshNames[28] = "Yamaha_Key_C3#";
    meshNames[29] = "Yamaha_Key_D3";
    meshNames[30] = "Yamaha_Key_D3#";
    meshNames[31] = "Yamaha_Key_E3";
    meshNames[32] = "Yamaha_Key_F3";
    meshNames[33] = "Yamaha_Key_F3#";
    meshNames[34] = "Yamaha_Key_G3";
    meshNames[35] = "Yamaha_Key_G3#";
    meshNames[36] = "Yamaha_Key_A3";
    meshNames[37] = "Yamaha_Key_A3#";
    meshNames[38] = "Yamaha_Key_B3";
    meshNames[39] = "Yamaha_Key_C4";
    meshNames[40] = "Yamaha_Key_C4#";
    meshNames[41] = "Yamaha_Key_D4";
    meshNames[42] = "Yamaha_Key_D4#";
    meshNames[43] = "Yamaha_Key_E4";
    meshNames[44] = "Yamaha_Key_F4";
    meshNames[45] = "Yamaha_Key_F4#";
    meshNames[46] = "Yamaha_Key_G4";
    meshNames[47] = "Yamaha_Key_G4#";
    meshNames[48] = "Yamaha_Key_A4";
    meshNames[49] = "Yamaha_Key_A4#";
    meshNames[50] = "Yamaha_Key_B4";
    meshNames[51] = "Yamaha_Key_C5";
    meshNames[52] = "Yamaha_Key_C5#";
    meshNames[53] = "Yamaha_Key_D5";
    meshNames[54] = "Yamaha_Key_D5#";
    meshNames[55] = "Yamaha_Key_E5";
    meshNames[56] = "Yamaha_Key_F5";
    meshNames[57] = "Yamaha_Key_F5#";
    meshNames[58] = "Yamaha_Key_G5";
    meshNames[59] = "Yamaha_Key_G5#";
    meshNames[60] = "Yamaha_Key_A5";
    meshNames[61] = "Yamaha_Key_A5#";
    meshNames[62] = "Yamaha_Key_B5";
    meshNames[63] = "Yamaha_Key_C6";
    meshNames[64] = "Yamaha_Key_C6#";
    meshNames[65] = "Yamaha_Key_D6";
    meshNames[66] = "Yamaha_Key_D6#";
    meshNames[67] = "Yamaha_Key_E6";
    meshNames[68] = "Yamaha_Key_F6";
    meshNames[69] = "Yamaha_Key_F6#";
    meshNames[70] = "Yamaha_Key_G6";
    meshNames[71] = "Yamaha_Key_G6#";
    meshNames[72] = "Yamaha_Key_A6";
    meshNames[73] = "Yamaha_Key_A6#";
    meshNames[74] = "Yamaha_Key_B6";
    meshNames[75] = "Yamaha_Key_C7";
    meshNames[76] = "Yamaha_Key_C7#";
    meshNames[77] = "Yamaha_Key_D7";
    meshNames[78] = "Yamaha_Key_D7#";
    meshNames[79] = "Yamaha_Key_E7";
    meshNames[80] = "Yamaha_Key_F7";
    meshNames[81] = "Yamaha_Key_F7#";
    meshNames[82] = "Yamaha_Key_G7";
    meshNames[83] = "Yamaha_Key_G7#";
    meshNames[84] = "Yamaha_Key_A7";
    meshNames[85] = "Yamaha_Key_A7#";
    meshNames[86] = "Yamaha_Key_B7";
    meshNames[87] = "Yamaha_Key_C8";

    for (int i = 0; i < meshNames.size(); i++) {
        if (meshNames[i] == meshName) {
            return i + 21;
        }
    }

    return 0;
}

void PianoKey::Update(float deltaTime, const std::string& soundFontName) {
    m_dirty = false;

    if (m_state == State::KEY_DOWN) {
        m_timeRemaining -= deltaTime;
        m_dirty = true;

        if (!m_isSharp) {
            m_yTranslation = 0.0f;
            m_xRotation = 0.05f;
        }
        else {
            m_yTranslation = -0.01f;
            m_xRotation = 0.0f;
        }

        // Done? Then reset transform and state
        if (m_timeRemaining <= 0) {
            m_timeRemaining = 0;
            m_xRotation = 0.0f;
            m_yTranslation = 0.0f;
            m_state = State::IDLE;
            if (Hell::Synth::SelectSoundFont(soundFontName)) {
                Hell::Synth::ReleaseNote(m_note);
            }
        }
    }
}

void PianoKey::PressKey(const std::string& soundFontName, int velocity, float duration) {
    // Play sound if you were not pressed already
    if (m_state == State::IDLE) {
        if (Hell::Synth::SelectSoundFont(soundFontName)) {
            Hell::Synth::PlayNote(m_note, velocity);
        }
    }

    m_state = State::KEY_DOWN;
    m_timeRemaining = duration;
}
}
