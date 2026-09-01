#pragma once
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"

#include "Unloved/Objects/Renderables/MeshNodes.h"

#include "Hell/Math/Transform.h"
#include "Hell/ResourceManagement/Types/Model.h"

#include <unordered_map>

namespace Unloved {

struct PianoKey {

    enum struct State {
        IDLE,
        KEY_DOWN
    };

    int32_t m_note = 0;
    glm::vec3 m_worldSpaceCenter = glm::vec3(0.0f);
    float m_xRotation = 0;
    float m_yTranslation = 0;
    bool m_isSharp = false;
    std::string m_meshName = "";
    State m_state = State::IDLE;

    void Update(float deltaTime, const std::string& soundFontName);
    void PressKey(const std::string& soundFontName, int velocity = 127, float duration = 0.05f);

    bool IsDirty() const { return m_dirty; }

private:
    float m_timeRemaining = 0.0f;
    bool m_dirty = true;
    bool m_sustain = false;
};

struct Piano {
    Piano() = default;
    Piano(uint64_t id, PianoCreateInfo createInfo);
    Piano(uint64_t id, PianoCreateInfo createInfo, SpawnOffset spawnOffset);

    void Init(uint64_t id, PianoCreateInfo createInfo);
    void Init(uint64_t id, PianoCreateInfo createInfo, SpawnOffset spawnOffset);
    void SetPosition(glm::vec3 position);
    void SetRotation(glm::vec3 rotation);
    void SetSustain(bool value);
    void Update(float deltaTime);
    void CleanUp();
    void TriggerInternalNoteFromExternalBulletHit(glm::vec3 bulletHitPositon);
    void CalculatePianoKeyWorldspaceCenters();
    void PressKey(uint32_t customId);

    void PlayMajorFirstInversion(int rootNote); 
    void PlayMajor7th(int rootNote);
    void PlayMinor(int rootNote);
    void PlayMajor(int rootNote);
    void PlayKey(int note, int velocity = 127, float duration = 0.05f);

    bool PianoKeyExists(uint64_t pianoKeyId);
	PianoKey* GetPianoKey(uint64_t pianoKeyId);

	MeshNodes& GetMeshNodes()                               { return m_meshNodes; }
    const bool IsDirty() const                              { return m_meshNodes.IsDirty(); }
    const std::vector<RenderItem>& GetRenderItems() const   { return m_meshNodes.GetRenderItems(); }
    const uint64_t& GetObjectId() const                     { return m_pianoObjectId; };
    const glm::vec3& GetPosition() const                    { return m_transform.position; }
    const glm::vec3& GetRotation() const                    { return m_transform.rotation; }
    const glm::vec3& GetSeatPosition() const                { return m_seatPosition; }
    const std::string& GetSoundFontName() const             { return m_soundFontName; }
    const PianoCreateInfo& GetCreateInfo() const            { return m_createInfo; }
    const std::string& GetEditorName() const                { return m_createInfo.editorName; }

    static uint32_t MeshNameToNote(const std::string& meshName);
        
private:
    glm::vec3 m_seatPosition = glm::vec3(0.0f);
    glm::mat4 m_worldMatrix;
    uint64_t m_pianoObjectId = 0;
    uint64_t m_rigidStaticId = 0;
    std::string m_soundFontName = UNDEFINED_STRING;
    Hell::Transform m_transform;
    PianoCreateInfo m_createInfo;
    MeshNodes m_meshNodes;
    std::unordered_map<uint32_t, PianoKey> m_keys;
};
}
