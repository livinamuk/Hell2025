#include "AssetManager.h"
#include "Util/Util.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <future>

namespace AssetManager {

    void LoadPendingAnimationsAsync() {
        for (Animation& animation : GetAnimations()) {
            if (animation.GetLoadingState() == LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                animation.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
                AddItemToLoadLog(animation.GetFileInfo().path);
                //std::async(std::launch::async, LoadAnimation, &animation);
                LoadAnimation(&animation);
                //return;
            }
        }
    }

    void LoadAnimation(Animation* animation) {
        const FileInfo& fileInfo = animation->GetFileInfo();

        Assimp::Importer m_AnimationImporter;

        // Try and load the animation
        const aiScene* animationScene = m_AnimationImporter.ReadFile(fileInfo.path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

        // Failed
        if (!animationScene) {
            std::cout << "Could not load: " << fileInfo.path << "\n";
            animation->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
            return;
        }

        if (animationScene->mNumAnimations == 0 || animationScene->mAnimations[0] == nullptr) {
            std::cout << "ATTENTION! " << animation->GetName() << " has zero animations\n";
            m_AnimationImporter.FreeScene();
            animation->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
            return;
        }

        animation->m_duration = (float)animationScene->mAnimations[0]->mDuration;
        animation->m_ticksPerSecond = (float)animationScene->mAnimations[0]->mTicksPerSecond;

        // need to create an animation clip.
        // need to fill it with animation poses.
        aiAnimation* aiAnim = animationScene->mAnimations[0];

        // Resize the vector big enough for each pose
        int nodeCount = (int)aiAnim->mNumChannels;
         // trying the assimp way now. coz why fight it.
        for (int n = 0; n < nodeCount; n++)
        {
            if (!aiAnim->mChannels[n]) {
                continue;
            }

            const char* nodeName = Util::CopyConstChar(aiAnim->mChannels[n]->mNodeName.C_Str());

            AnimatedNode animatedNode(nodeName);
            animation->m_NodeMapping.emplace(nodeName, n);

            const aiNodeAnim* channel = aiAnim->mChannels[n];
            unsigned int numPosKeys = channel->mNumPositionKeys;
            unsigned int numRotKeys = channel->mNumRotationKeys;
            unsigned int numScaleKeys = channel->mNumScalingKeys;
            unsigned int keyCount = std::max({ numPosKeys, numRotKeys, numScaleKeys });

            if (keyCount == 0) {
                continue;
            }

            for (unsigned int p = 0; p < keyCount; ++p)
            {
                SQT sqt;
                sqt.positon = glm::vec3(0.0f);
                sqt.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                sqt.scale = glm::vec3(1.0f);
                sqt.timeStamp = 0.0f;

                if (numPosKeys > 0) {
                    const aiVectorKey& pos = channel->mPositionKeys[std::min(p, numPosKeys - 1)];
                    sqt.positon = glm::vec3(pos.mValue.x, pos.mValue.y, pos.mValue.z);
                    sqt.timeStamp = (float)pos.mTime;
                }

                if (numRotKeys > 0) {
                    const aiQuatKey& rot = channel->mRotationKeys[std::min(p, numRotKeys - 1)];
                    sqt.rotation = glm::quat(rot.mValue.w, rot.mValue.x, rot.mValue.y, rot.mValue.z);
                    sqt.timeStamp = std::max(sqt.timeStamp, (float)rot.mTime);
                }

                if (numScaleKeys > 0) {
                    const aiVectorKey& scale = channel->mScalingKeys[std::min(p, numScaleKeys - 1)];
                    sqt.scale = glm::vec3(scale.mValue.x, scale.mValue.y, scale.mValue.z);
                    sqt.timeStamp = std::max(sqt.timeStamp, (float)scale.mTime);
                }

                // not good: sqt.positon = Util::SanitizeVec3(sqt.positon);
                // not good: sqt.rotation = Util::SanitizeQuat(sqt.rotation);
                // not good: sqt.scale = Util::SanitizeVec3(sqt.scale);

                animation->m_finalTimeStamp = std::max(animation->m_finalTimeStamp, sqt.timeStamp);

                animatedNode.m_nodeKeys.push_back(sqt);
            }
            animation->m_animatedNodes.push_back(animatedNode);
        }
        // Store it
        m_AnimationImporter.FreeScene();

        animation->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        //animation->PrintNodeNames();
        //std::cout << "\n";
    }

    Animation* AssetManager::GetAnimationByName(const std::string& name) {
        std::vector<Animation>& animations = GetAnimations();
        for (auto& animation : animations) {
            if (name == animation.GetName()) {
                return &animation;
            }
        }
        std::cout << "AssetManager::GetAnimationByName(const std::string& name) failed because '" << name << "' does not exist!\n";
        return nullptr;
    }

    Animation* AssetManager::GetAnimationByIndex(int index, bool printError) {
        std::vector<Animation>& animations = GetAnimations();
        if (index >= 0 && index < animations.size()) {
            return &animations[index];
        }
        else {
            if (printError) {
                std::cout << "AssetManager::GetAnimationByIndex(int index) failed because index '" << index << "' is out of range. Size is " << animations.size() << "!\n";
            }
            return nullptr;
        }
    }

    int AssetManager::GetAnimationIndexByName(const std::string& name) {
        std::vector<Animation>& animations = GetAnimations();
        for (int i = 0; i < animations.size(); i++) {
            if (name == animations[i].GetName()) {
                return i;
            }
        }
        std::cout << "AssetManager::GetAnimationIndexByName(const std::string& name) failed because '" << name << "' does not exist!\n";
        return -1;
    }
}
