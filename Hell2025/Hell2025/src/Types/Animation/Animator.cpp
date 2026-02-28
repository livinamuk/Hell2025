#include "Animator.h"
#include "AssetManagement/AssetManager.h"
#include "Util.h"

void Animator::SetSkinnedModel(const std::string& skinnedModelName) {
    m_skinnedModel= AssetManager::GetSkinnedModelByName(skinnedModelName);
}

void Animator::PlayAnimation(const std::string& layerName, const std::string& animationName, float speed, bool loop) {
    Animation* animation = AssetManager::GetAnimationByName(animationName);
    if (!animation) return;

    // Create layer if it doesn't exist
    if (!AnimationLayerExists(layerName)) {
        CreateAnimationLayer(layerName);
    }

    AnimationLayer* animationLayer = GetAnimationLayerByName(layerName);
    if (!animationLayer) return;

    // Bail if told to loop and already playing this animation
    if (loop && animationLayer->m_animation == animation) {
        return;
    }

    int nodeCount = animation->m_animatedNodes.size();

    animationLayer->m_animation = animation;
    animationLayer->m_globalNodeTransforms.resize(nodeCount);
    animationLayer->m_boneWeights.resize(nodeCount);
    animationLayer->m_animationSpeed = speed;
    animationLayer->m_currentTime = 0.0f;
    animationLayer->m_isComplete = false;
    animationLayer->m_paused = false;
    animationLayer->m_loop = loop;

    // Set default weight for each bone to 1.0 and transform to identity
    for (int i = 0; i < nodeCount; i++) {
        animationLayer->m_globalNodeTransforms[i] = glm::mat4(1.0f);
        animationLayer->m_boneWeights[i] = 1.0f;
    }
}

void Animator::PlayAndLoopAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    PlayAnimation(layerName, animationName, speed, true);
}

void Animator::CreateAnimationLayer(const std::string& name) {
    if (AnimationLayerExists(name)) return;

    int nodeCount = m_skinnedModel->GetNodeCount();

    m_animationLayers[name] = AnimationLayer();
    m_animationLayers[name].m_AnimationWeight = 1.0f;
    m_animationLayers[name].m_boneWeights.resize(nodeCount);

    for (int i = 0; i < nodeCount; i++) {
        m_animationLayers[name].m_boneWeights[i] = 1.0f;
    }
}

void Animator::ClearAllAnimations() {
    m_animationLayers.clear();
}

void Animator::PauseAllLayers() {
    for (auto& [name, animationLayer] : m_animationLayers) {
        animationLayer.m_paused = true;
    }
}

void Animator::SetAdditiveTransform(const std::string& nodeName, const glm::mat4& matrix) {
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    m_additiveNodeTransforms[nodeName] = matrix;
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
    // TODO: Make sure this bone name is valid
}

bool Animator::AnimationLayerExists(const std::string& name) const {
    return m_animationLayers.find(name) != m_animationLayers.end();
}

AnimationLayer* Animator::GetAnimationLayerByName(const std::string& name) {
    auto it = m_animationLayers.find(name);
    if (it == m_animationLayers.end()) return nullptr;
    return &it->second;
}

void Animator::UpdateAnimations(float deltaTime) {
    // Bail if invalid skinned model
    if (!m_skinnedModel) return;

    // First compute all animations individually
    for (auto& [name, animationLayer] : m_animationLayers) {
        UpdateAnimation(animationLayer, deltaTime);
    }

    int nodeCount = m_skinnedModel->GetNodeCount();

    std::vector<AnimatedTransform> finalLocals(nodeCount);
    std::vector<float> weightSum(nodeCount, 0.0f);

    // Clear output and weight accumulators
    for (int i = 0; i < nodeCount; ++i) {
        weightSum[i] = 0.0f;
        finalLocals[i].translation = glm::vec3(0.0f);
        finalLocals[i].scale = glm::vec3(0.0f);
        finalLocals[i].rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // Now blend all animation states
    for (auto& [name, animationLayer] : m_animationLayers) {

        // Skip if layer weight too low
        if (animationLayer.m_AnimationWeight <= 0.00001f) continue;

        for (int i = 0; i < nodeCount; ++i) {
            float w = animationLayer.m_AnimationWeight * animationLayer.m_boneWeights[i];

            // Skip if bone weight too low
            if (w <= 0.00001f) continue;

            float newWeightSum = weightSum[i] + w;
            float blendFactor = w / newWeightSum;

            // Linearly blend translation and scale
            finalLocals[i].translation += animationLayer.m_localNodeTransforms[i].translation * w;
            finalLocals[i].scale += animationLayer.m_localNodeTransforms[i].scale * w;

            // Slerp rotation
            glm::quat current = glm::normalize(finalLocals[i].rotation);
            glm::quat target = animationLayer.m_localNodeTransforms[i].rotation;

            // Fix inverted rotations
            if (glm::dot(current, target) < 0.0f) {
                target = -target;
            }

            finalLocals[i].rotation = glm::slerp(current, target, blendFactor);
            finalLocals[i].rotation = glm::normalize(finalLocals[i].rotation);

            weightSum[i] = newWeightSum;
        }
    }

    // Normalize translation and scale by total weight
    for (int i = 0; i < nodeCount; ++i) {
        if (weightSum[i] > 1e-5f) {
            float inv = 1.0f / weightSum[i];
            finalLocals[i].translation *= inv;
            finalLocals[i].scale *= inv;
            finalLocals[i].rotation = glm::normalize(finalLocals[i].rotation);
        }
        else {
            // Fallback to bind pose
            finalLocals[i] = m_skinnedModel->m_nodes[i].inverseBindTransform;
        }
    }

    m_LocalBlendedBoneTransforms.resize(nodeCount);
    m_globalBlendedNodeTransforms.resize(nodeCount);

    for (int i = 0; i < nodeCount; ++i) {
        glm::mat4 localMatrix = finalLocals[i].to_mat4();

        // Apply additive matrix
        std::string& nodeName = m_skinnedModel->m_nodes[i].name;
        auto it = m_additiveNodeTransforms.find(nodeName);
        if (it != m_additiveNodeTransforms.end()) {
            localMatrix = it->second * localMatrix;
        }

        // Store final local
        m_LocalBlendedBoneTransforms[i] = localMatrix;

        // Store final global
        int parent = m_skinnedModel->m_nodes[i].parentIndex;
        if (parent >= 0) {
            m_globalBlendedNodeTransforms[i] = m_globalBlendedNodeTransforms[parent] * localMatrix;
        }
        else {
            m_globalBlendedNodeTransforms[i] = localMatrix;
        }
    }

    // Nuke any rounding errors
    for (glm::mat4& m : m_globalBlendedNodeTransforms) {
        Util::SanitizeMat4(m);
    }
}

void Animator::UpdateAnimation(AnimationLayer& animationLayer, float deltaTime) {
    if (!m_skinnedModel) return;
    if (!animationLayer.m_animation) return;

    int nodeCount = m_skinnedModel->GetNodeCount();

    // Ensure transforms match the number of nodes
    animationLayer.m_globalNodeTransforms.resize(nodeCount);
    animationLayer.m_localNodeTransforms.resize(nodeCount);

    // Update current animation time based on deltaTime
    float timeInTicks = 0;
    if (!animationLayer.m_paused) {
        animationLayer.m_currentTime += deltaTime * animationLayer.m_animationSpeed;
    }
    float duration = animationLayer.m_animation->m_duration / animationLayer.m_animation->m_ticksPerSecond;
    if (animationLayer.m_currentTime >= duration) {
        if (!animationLayer.m_loop) {
            animationLayer.m_currentTime = duration;
            animationLayer.m_paused = true;
            animationLayer.m_isComplete = true;
        }
        else {
            animationLayer.m_currentTime = 0;
            //animationLayer.m_currentTime = fmod(animationLayer.m_currentTime, duration);
        }
    }
    timeInTicks = GetAnimationTimeInTicks(animationLayer);


    // Traverse the tree
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        glm::mat4 nodeTransformation = glm::mat4(1.0f);// m_skinnedModel->m_nodes[i].inverseBindTransform;
        std::string& nodeName = m_skinnedModel->m_nodes[i].name;

        // Interpolate the node transformation if it's animated
        if (animationLayer.m_animation) {
            const AnimatedNode* animatedNode = FindAnimatedNode(animationLayer.m_animation, nodeName.c_str());
            if (animatedNode) {
                glm::vec3 translation;
                glm::quat rotation;
                glm::vec3 scale;
                Util::CalcInterpolatedPosition(translation, timeInTicks, animatedNode);
                Util::CalcInterpolatedRotation(rotation, timeInTicks, animatedNode);
                Util::CalcInterpolatedScale(scale, timeInTicks, animatedNode);
                nodeTransformation = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation);
                nodeTransformation = glm::scale(nodeTransformation, scale);
            }
        }
        else {
            nodeTransformation = m_skinnedModel->m_nodes[i].inverseBindTransform;
        }

        animationLayer.m_localNodeTransforms[i] = AnimatedTransform(nodeTransformation);
    }

    // Rebuild global matrices from the locals
    for (int i = 0; i < nodeCount; ++i) {
        glm::mat4 localM = animationLayer.m_localNodeTransforms[i].to_mat4();
        int parent = m_skinnedModel->m_nodes[i].parentIndex;
        glm::mat4 worldM = (parent >= 0) ? animationLayer.m_globalNodeTransforms[parent].to_mat4() * localM : localM;
        animationLayer.m_globalNodeTransforms[i] = AnimatedTransform(worldM);
    }
}

float Animator::GetAnimationTimeInTicks(AnimationLayer& animationState) {
    Animation* animation = animationState.m_animation;
    if (!animation) return 0;
    if (animation->m_duration <= 0.0f) return 0;

    float ticksPerSecond = animation->m_ticksPerSecond != 0 ? animation->m_ticksPerSecond : 25.0f;
    float timeInTicks = animationState.m_currentTime * ticksPerSecond;
    if (timeInTicks == animation->m_duration) {
        return timeInTicks;
    }
    else {
        timeInTicks = fmod(timeInTicks, animation->m_duration);
        timeInTicks = std::min(timeInTicks, animation->m_duration);
        return timeInTicks;
    }
    return 0;
}

const AnimatedNode* Animator::FindAnimatedNode(Animation* animation, const char* NodeName) {
    for (unsigned int i = 0; i < animation->m_animatedNodes.size(); i++) {
        const AnimatedNode* animatedNode = &animation->m_animatedNodes[i];

        if (Util::StrCmp(animatedNode->m_nodeName, NodeName)) {
            return animatedNode;
        }
    }
    return nullptr;
}

bool Animator::AnimationIsCompleteAnyLayer(const std::string& animationName) {
    for (auto& [name, animationLayer] : m_animationLayers) {
        if (animationLayer.m_animation->GetName() == animationName && animationLayer.m_isComplete) {
            return true;
        }
    }
    return false;
}

bool Animator::AllAnimationsComplete() {
    for (auto& [name, animationLayer] : m_animationLayers) {
        if (!animationLayer.m_isComplete) {
            return false;
        }
    }
    return true;
}

uint32_t Animator::GetAnimationFrameNumber(const std::string& animationLayerName) {
    AnimationLayer* animationLayer = GetAnimationLayerByName(animationLayerName);
    if (!animationLayer) return 0;
    if (!animationLayer->m_animation) return 0;

    return animationLayer->m_currentTime * animationLayer->m_animation->m_ticksPerSecond;
}

bool Animator::AnimationIsPastFrameNumber(const std::string& animationLayerName, int frameNumber) {
    AnimationLayer* animationLayer = GetAnimationLayerByName(animationLayerName);
    if (!animationLayer) return false;

    uint32_t currentFrame = uint32_t(animationLayer->m_currentTime * animationLayer->m_animation->m_ticksPerSecond);
    return currentFrame > uint32_t(frameNumber);
}
