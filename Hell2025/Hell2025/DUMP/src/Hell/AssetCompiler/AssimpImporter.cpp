#include "AssimpImporter.h"

#include "Hell/Common/String.h"
#include "Hell/File.h"
#include "Hell/Math/Math.h"

#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>
#include <assimp/Scene.h>
#include <assimp/PostProcess.h>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <utility>

#include <iostream> // TODO: cleanup logging

namespace {

    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
        constexpr float threshold = 1e-5f;
        glm::mat4 to;
        to[0][0] = Hell::Math::Sanitize(from.a1, threshold); to[1][0] = Hell::Math::Sanitize(from.a2, threshold); to[2][0] = Hell::Math::Sanitize(from.a3, threshold); to[3][0] = Hell::Math::Sanitize(from.a4, threshold);
        to[0][1] = Hell::Math::Sanitize(from.b1, threshold); to[1][1] = Hell::Math::Sanitize(from.b2, threshold); to[2][1] = Hell::Math::Sanitize(from.b3, threshold); to[3][1] = Hell::Math::Sanitize(from.b4, threshold);
        to[0][2] = Hell::Math::Sanitize(from.c1, threshold); to[1][2] = Hell::Math::Sanitize(from.c2, threshold); to[2][2] = Hell::Math::Sanitize(from.c3, threshold); to[3][2] = Hell::Math::Sanitize(from.c4, threshold);
        to[0][3] = Hell::Math::Sanitize(from.d1, threshold); to[1][3] = Hell::Math::Sanitize(from.d2, threshold); to[2][3] = Hell::Math::Sanitize(from.d3, threshold); to[3][3] = Hell::Math::Sanitize(from.d4, threshold);
        return to;
    }

}

namespace Hell::AssetCompiler {

    namespace {
        constexpr float BONE_WEIGHT_EPSILON = 1e-6f;
        constexpr float MORPH_DELTA_EPSILON_SQUARED = 1e-12f;
        constexpr std::size_t MAX_BONE_INFLUENCES = 4;

        struct BoneInfluence {
            unsigned int boneIndex = 0;
            float weight = 0.0f;
        };

        struct SkeletonPruneData {
            std::vector<std::string> sourceBoneNames;
            std::unordered_map<std::string, glm::mat4> boneOffsets;
            std::unordered_set<std::string> weightedBoneNames;
            std::unordered_set<const aiNode*> retainedNodes;
            const aiNode* relevantSubtreeRoot = nullptr;
        };

        std::string ResolveRequestedMorphTargetName(
            const std::string& assimpMorphTargetName,
            const std::unordered_set<std::string>& requestedMorphTargets) {
            if (requestedMorphTargets.contains(assimpMorphTargetName)) {
                return assimpMorphTargetName;
            }

            // Assimp's FBX importer can expose a blend-shape channel and its
            // target as "Channel.Target". Blender commonly gives both parts
            // the same name, for example "Eye_Blink_L.Eye_Blink_L".
            const std::size_t separator = assimpMorphTargetName.rfind('.');
            if (separator == std::string::npos || separator + 1 >= assimpMorphTargetName.size()) {
                return {};
            }

            const std::string targetName = assimpMorphTargetName.substr(separator + 1);
            return requestedMorphTargets.contains(targetName) ? targetName : std::string{};
        }

        bool BoneHasRelevantWeight(const aiBone* bone) {
            for (unsigned int i = 0; i < bone->mNumWeights; i++) {
                if (bone->mWeights[i].mWeight > BONE_WEIGHT_EPSILON) {
                    return true;
                }
            }
            return false;
        }

        void CollectNodeSubtree(const aiNode* node, std::unordered_set<const aiNode*>& nodes) {
            if (!node || !nodes.insert(node).second) {
                return;
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                CollectNodeSubtree(node->mChildren[i], nodes);
            }
        }

        void CollectNodesByName(const aiNode* node, std::unordered_map<std::string, std::vector<const aiNode*>>& nodesByName) {
            if (!node) {
                return;
            }

            nodesByName[node->mName.C_Str()].push_back(node);
            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                CollectNodesByName(node->mChildren[i], nodesByName);
            }
        }

        const aiNode* FindBoneNode(const std::unordered_map<std::string, std::vector<const aiNode*>>& nodesByName, const std::string& boneName) {
            const auto namedNodes = nodesByName.find(boneName);
            if (namedNodes == nodesByName.end()) {
                return nullptr;
            }

            const aiNode* boneNode = nullptr;
            for (const aiNode* candidate : namedNodes->second) {
                if (candidate->mNumMeshes > 0) {
                    continue;
                }
                if (boneNode) {
                    return nullptr;
                }
                boneNode = candidate;
            }
            return boneNode;
        }

        const aiNode* FindLowestCommonAncestor(const std::vector<const aiNode*>& nodes) {
            if (nodes.empty()) {
                return nullptr;
            }

            const aiNode* commonAncestor = nodes.front();
            for (size_t i = 1; i < nodes.size() && commonAncestor; i++) {
                std::unordered_set<const aiNode*> ancestors;
                for (const aiNode* ancestor = nodes[i]; ancestor; ancestor = ancestor->mParent) {
                    ancestors.insert(ancestor);
                }

                while (commonAncestor && ancestors.find(commonAncestor) == ancestors.end()) {
                    commonAncestor = commonAncestor->mParent;
                }
            }
            return commonAncestor;
        }

        bool BuildSkeletonPruneData(const aiScene* scene, const std::string& filepath, SkeletonPruneData& outData) {
            // Weighted bones decide which skeleton branch belongs to this mesh
            std::unordered_set<std::string> discoveredBoneNames;
            std::unordered_map<std::string, std::vector<const aiNode*>> nodesByName;
            CollectNodesByName(scene->mRootNode, nodesByName);

            for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
                const aiMesh* mesh = scene->mMeshes[meshIndex];
                for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
                    const aiBone* bone = mesh->mBones[boneIndex];
                    const std::string boneName = bone->mName.C_Str();

                    if (discoveredBoneNames.insert(boneName).second) {
                        outData.sourceBoneNames.push_back(boneName);
                        outData.boneOffsets.emplace(boneName, aiMatrix4x4ToGlm(bone->mOffsetMatrix));
                    }
                    if (BoneHasRelevantWeight(bone)) {
                        outData.weightedBoneNames.insert(boneName);
                    }
                }
            }

            if (outData.weightedBoneNames.empty()) {
                std::cout << "ImportSkinnedModel() found no weighted bones in '" << filepath << "'\n";
                return false;
            }

            std::vector<const aiNode*> weightedBoneNodes;
            weightedBoneNodes.reserve(outData.weightedBoneNames.size());
            for (const std::string& boneName : outData.weightedBoneNames) {
                const aiNode* boneNode = FindBoneNode(nodesByName, boneName);
                if (!boneNode) {
                    std::cout << "ImportSkinnedModel() could not find a unique non-mesh node for weighted bone '" << boneName << "' in '" << filepath << "'\n";
                    return false;
                }
                weightedBoneNodes.push_back(boneNode);
            }

            outData.relevantSubtreeRoot = FindLowestCommonAncestor(weightedBoneNodes);
            if (!outData.relevantSubtreeRoot) {
                std::cout << "ImportSkinnedModel() could not find a common weighted-bone ancestor in '" << filepath << "'\n";
                return false;
            }

            // Keep the complete relevant branch including unweighted sockets and helper bones
            CollectNodeSubtree(outData.relevantSubtreeRoot, outData.retainedNodes);

            // Keep the chain to the Assimp scene root so node paths stay stable
            for (const aiNode* ancestor = outData.relevantSubtreeRoot->mParent; ancestor; ancestor = ancestor->mParent) {
                outData.retainedNodes.insert(ancestor);
            }

            return true;
        }

        void GrabSkeleton(std::vector<Node>& nodes, const aiNode* node, int parentIndex, const std::unordered_set<const aiNode*>& retainedNodes) {
            if (retainedNodes.find(node) == retainedNodes.end()) {
                return;
            }

            // Skip mesh nodes so they cannot collide with bone names
            // Keep processing children so the retained hierarchy stays intact
            if (node->mNumMeshes > 0) {
                for (unsigned int i = 0; i < node->mNumChildren; i++) {
                    GrabSkeleton(nodes, node->mChildren[i], parentIndex, retainedNodes);
                }
                return;
            }

            Node outputNode;
            outputNode.name = node->mName.C_Str();
            outputNode.localBindTransform = aiMatrix4x4ToGlm(node->mTransformation);
            outputNode.parentIndex = parentIndex;

            const int currentIndex = static_cast<int>(nodes.size());
            nodes.push_back(outputNode);

            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                GrabSkeleton(nodes, node->mChildren[i], currentIndex, retainedNodes);
            }
        }
    }

    static ModelData ImportStaticModel(const std::string& filepath, unsigned int flags, bool generateTangents = true) {
        ModelData modelData;
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
        const aiScene* scene = importer.ReadFile(filepath, flags);
        if (!scene) {
            std::cout << "LoadAndExportCustomFormat() failed to loaded model " << filepath << "\n";
            std::cerr << "Assimp Error: " << importer.GetErrorString() << "\n";
            return modelData;
        }
        modelData.name = File::GetName(filepath);
        modelData.meshCount = scene->mNumMeshes;
        modelData.meshes.resize(modelData.meshCount);
        modelData.timestamp = File::GetLastModifiedTime(filepath);

        // Pre allocate vector memory
        std::unordered_map<std::string, int> meshNameCounts;
        for (int i = 0; i < modelData.meshes.size(); i++) {
            MeshData& meshData = modelData.meshes[i];
            meshData.vertexCount = scene->mMeshes[i]->mNumVertices;
            meshData.indexCount = scene->mMeshes[i]->mNumFaces * 3;
            meshData.vertices.resize(meshData.vertexCount);
            meshData.indices.resize(meshData.indexCount);

            std::string rawName = scene->mMeshes[i]->mName.C_Str();
            // Remove blender naming mess
            rawName = rawName.substr(0, rawName.find('.'));

            meshNameCounts[rawName]++;
            if (meshNameCounts[rawName] > 1) {
                meshData.name = rawName + std::to_string(meshNameCounts[rawName]);
            }
            else {
                meshData.name = rawName;
            }
        }
        // Populate vectors
        for (int i = 0; i < modelData.meshes.size(); i++) {
            MeshData& meshData = modelData.meshes[i];
            const aiMesh* assimpMesh = scene->mMeshes[i];

            // Vertices
            for (unsigned int j = 0; j < meshData.vertexCount; j++) {
                meshData.vertices[j] = (Vertex{
                    // Pos
                    glm::vec3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z),
                    // Normal
                    glm::vec3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z),
                    // UV
                    assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f),
                    // Tangent
                    assimpMesh->HasTangentsAndBitangents() ? glm::vec3(assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z) : glm::vec3(0.0f)
                    });
                // Compute AABB
                meshData.aabbMin = glm::min(meshData.vertices[j].position, meshData.aabbMin);
                meshData.aabbMax = glm::max(meshData.vertices[j].position, meshData.aabbMax);
            }

            // Get indices
            for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
                const aiFace& face = assimpMesh->mFaces[j];
                unsigned int baseIndex = j * 3;
                meshData.indices[baseIndex] = face.mIndices[0];
                meshData.indices[baseIndex + 1] = face.mIndices[1];
                meshData.indices[baseIndex + 2] = face.mIndices[2];
            }

            // Normalize the normals for each vertex
            for (Vertex& vertex : meshData.vertices) {
                vertex.normal = glm::normalize(vertex.normal);
            }

            if (generateTangents) {
                // Generate Tangents
                for (int j = 0; j < meshData.indices.size(); j += 3) {
                    Vertex* vert0 = &meshData.vertices[meshData.indices[j]];
                    Vertex* vert1 = &meshData.vertices[meshData.indices[j + 1]];
                    Vertex* vert2 = &meshData.vertices[meshData.indices[j + 2]];
                    glm::vec3 deltaPos1 = vert1->position - vert0->position;
                    glm::vec3 deltaPos2 = vert2->position - vert0->position;
                    glm::vec2 deltaUV1 = vert1->uv - vert0->uv;
                    glm::vec2 deltaUV2 = vert2->uv - vert0->uv;
                    const float determinant = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
                    if (std::abs(determinant) < 1e-8f) {
                        continue;
                    }
                    float r = 1.0f / determinant;
                    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
                    vert0->tangent = tangent;
                    vert1->tangent = tangent;
                    vert2->tangent = tangent;
                }
            }

            modelData.aabbMin = glm::min(modelData.aabbMin, meshData.aabbMin);
            modelData.aabbMax = glm::max(modelData.aabbMax, meshData.aabbMax);
        }
        importer.FreeScene();
        return modelData;
    }

    ModelData ImportModel(const std::string& filepath) {
        return ImportStaticModel(filepath,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_FlipUVs
        );
    }

    ModelData ImportVatCarrierModel(const std::string& filepath) {
        return ImportStaticModel(filepath,
            aiProcess_Triangulate,
            false
        );
    }

    void GrabSkeleton2(std::vector<Node>& nodes, const aiNode* pNode, int parentIndex) {
        // Create the joint node
        Node node;
        node.name = pNode->mName.C_Str();
        node.localBindTransform = aiMatrix4x4ToGlm(pNode->mTransformation);
        node.parentIndex = parentIndex;

        // Determine the current node's index and push it
        int currentIndex = static_cast<int>(nodes.size());
        nodes.push_back(node);

        // Recursively process children using the current node's index as parentIndex
        for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
            GrabSkeleton2(nodes, pNode->mChildren[i], currentIndex);
        }
    }

    SkinnedModelData ImportSkinnedModel(const std::string& filepath, const std::unordered_set<std::string>& requestedMorphTargets) {
        SkinnedModelData modelData;

        unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace;

        // NEW_RIG_FILE
        const std::string modelName = File::GetName(filepath);
        if (modelName == "Knife" ||
            modelName == "Tokarev" ||
            modelName == "GoldenGlock" ||
            modelName == "SPAS" ||
            modelName == "P90" ||
            modelName == "AKS74U" ||
            modelName == "Remington870") {
            flags =
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace |
                aiProcess_GlobalScale; // This list adds this flag
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath.c_str(), flags);

        if (!scene) {
            std::cout << "Something fucked up loading your skinned model: " << filepath << "\n";
            std::cout << "Error: " << importer.GetErrorString() << "\n";
            return modelData;
        }

        modelData.name = File::GetName(filepath);
        modelData.meshes.resize(scene->mNumMeshes);
        modelData.timestamp = File::GetLastModifiedTime(filepath);
        modelData.vertexCount = 0u;
        modelData.indexCount = 0u;

        SkeletonPruneData skeletonPruneData;
        if (!BuildSkeletonPruneData(scene, filepath, skeletonPruneData)) {
            importer.FreeScene();
            return {};
        }

        // Unweighted bones stay as skeleton nodes but do not enter the skinning palette
        for (const std::string& boneName : skeletonPruneData.sourceBoneNames) {
            if (skeletonPruneData.weightedBoneNames.find(boneName) == skeletonPruneData.weightedBoneNames.end()) {
                continue;
            }

            const unsigned int boneIndex = static_cast<unsigned int>(modelData.boneOffsets.size());
            modelData.boneMapping[boneName] = boneIndex;
            modelData.boneOffsets.push_back(skeletonPruneData.boneOffsets.at(boneName));
        }

        GrabSkeleton(modelData.nodes, scene->mRootNode, -1, skeletonPruneData.retainedNodes);

        // Get vertex data
        std::unordered_map<std::string, int> meshNameCounts;
        std::unordered_set<std::string> importedMorphTargetNames;
        uint64_t importedMorphTargetCount = 0;
        uint64_t importedMorphPositionDeltaCount = 0;
        uint64_t importedMorphNormalDeltaCount = 0;
        uint64_t importedMorphTangentDeltaCount = 0;
        for (int i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* assimpMesh = scene->mMeshes[i];

            SkinnedMeshData& meshData = modelData.meshes[i];;
            meshData.aabbMin = glm::vec3(std::numeric_limits<float>::max());
            meshData.aabbMax = glm::vec3(-std::numeric_limits<float>::max());
            meshData.vertexCount = assimpMesh->mNumVertices;
            meshData.indexCount = assimpMesh->mNumFaces * 3;

            std::string rawName = assimpMesh->mName.C_Str();
            meshNameCounts[rawName]++;
            if (meshNameCounts[rawName] > 1) {
                meshData.name = rawName + std::to_string(meshNameCounts[rawName]);
            }
            else {
                meshData.name = rawName;
            }

            meshData.vertices.reserve(meshData.vertexCount);
            meshData.vertexWeights.resize(meshData.vertexCount);
            meshData.indices.reserve(meshData.indexCount);

            // Get vertices
            for (unsigned int j = 0; j < meshData.vertexCount; j++) {
                Vertex& vertex = meshData.vertices.emplace_back();
                vertex.position = { assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z };
                vertex.normal = { assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z };
                vertex.normal = glm::normalize(vertex.normal);

                // avoid segfault if mesh lacks uvs and normalize to fix float drift
                if (assimpMesh->HasTangentsAndBitangents()) {
                    vertex.tangent = { assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z };
                    vertex.tangent = glm::normalize(vertex.tangent);
                }
                else {
                    vertex.tangent = glm::vec3(0.0f);
                }

                vertex.uv = { assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f) };

                meshData.aabbMin.x = std::min(meshData.aabbMin.x, vertex.position.x);
                meshData.aabbMin.y = std::min(meshData.aabbMin.y, vertex.position.y);
                meshData.aabbMin.z = std::min(meshData.aabbMin.z, vertex.position.z);
                meshData.aabbMax.x = std::max(meshData.aabbMax.x, vertex.position.x);
                meshData.aabbMax.y = std::max(meshData.aabbMax.y, vertex.position.y);
                meshData.aabbMax.z = std::max(meshData.aabbMax.z, vertex.position.z);
            }

            // Shape keys arrive from Assimp as full replacement meshes. Keep only
            // explicitly requested targets and store sparse position deltas so a
            // facial target does not duplicate every vertex in the source mesh.
            for (unsigned int morphIndex = 0; morphIndex < assimpMesh->mNumAnimMeshes; morphIndex++) {
                const aiAnimMesh* assimpMorphTarget = assimpMesh->mAnimMeshes[morphIndex];
                if (!assimpMorphTarget || !assimpMorphTarget->mVertices) continue;

                const std::string assimpMorphTargetName = assimpMorphTarget->mName.C_Str();
                const std::string morphTargetName = ResolveRequestedMorphTargetName(assimpMorphTargetName, requestedMorphTargets);
                if (morphTargetName.empty()) continue;

                if (assimpMorphTarget->mNumVertices != assimpMesh->mNumVertices) {
                    std::cout << "ImportSkinnedModel() found morph target '" << morphTargetName
                              << "' with a mismatched vertex count in mesh '" << meshData.name
                              << "' from '" << filepath << "'\n";
                    importer.FreeScene();
                    return {};
                }

                MorphTargetData morphTarget;
                morphTarget.name = morphTargetName;

                for (uint32_t vertexIndex = 0; vertexIndex < assimpMorphTarget->mNumVertices; vertexIndex++) {
                    const aiVector3D& basePosition = assimpMesh->mVertices[vertexIndex];
                    const aiVector3D& targetPosition = assimpMorphTarget->mVertices[vertexIndex];
                    const glm::vec3 positionDelta = {
                        targetPosition.x - basePosition.x,
                        targetPosition.y - basePosition.y,
                        targetPosition.z - basePosition.z
                    };

                    if (glm::dot(positionDelta, positionDelta) > MORPH_DELTA_EPSILON_SQUARED) {
                        morphTarget.positionDeltas.push_back({ vertexIndex, positionDelta });
                    }

                    if (assimpMorphTarget->mNormals && assimpMesh->mNormals) {
                        const aiVector3D& baseNormal = assimpMesh->mNormals[vertexIndex];
                        const aiVector3D& targetNormal = assimpMorphTarget->mNormals[vertexIndex];
                        const glm::vec3 normalDelta = {
                            targetNormal.x - baseNormal.x,
                            targetNormal.y - baseNormal.y,
                            targetNormal.z - baseNormal.z
                        };

                        if (glm::dot(normalDelta, normalDelta) > MORPH_DELTA_EPSILON_SQUARED) {
                            morphTarget.normalDeltas.push_back({ vertexIndex, normalDelta });
                        }
                    }

                    if (assimpMorphTarget->mTangents && assimpMesh->mTangents) {
                        const aiVector3D& baseTangent = assimpMesh->mTangents[vertexIndex];
                        const aiVector3D& targetTangent = assimpMorphTarget->mTangents[vertexIndex];
                        const glm::vec3 tangentDelta = {
                            targetTangent.x - baseTangent.x,
                            targetTangent.y - baseTangent.y,
                            targetTangent.z - baseTangent.z
                        };

                        if (glm::dot(tangentDelta, tangentDelta) > MORPH_DELTA_EPSILON_SQUARED) {
                            morphTarget.tangentDeltas.push_back({ vertexIndex, tangentDelta });
                        }
                    }
                }

                if (!morphTarget.positionDeltas.empty() ||
                    !morphTarget.normalDeltas.empty() ||
                    !morphTarget.tangentDeltas.empty()) {
                    importedMorphTargetNames.insert(morphTarget.name);
                    importedMorphTargetCount++;
                    importedMorphPositionDeltaCount += morphTarget.positionDeltas.size();
                    importedMorphNormalDeltaCount += morphTarget.normalDeltas.size();
                    importedMorphTangentDeltaCount += morphTarget.tangentDeltas.size();
                    meshData.morphTargets.push_back(std::move(morphTarget));
                }
            }

            // Get indices
            for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
                const aiFace& Face = assimpMesh->mFaces[j];
                meshData.indices.push_back(Face.mIndices[0]);
                meshData.indices.push_back(Face.mIndices[1]);
                meshData.indices.push_back(Face.mIndices[2]);
            }

            // Gather every source influence so we can select the strongest four
            // deterministically. Assimp's LimitBoneWeights pass is deliberately
            // not used: it hides which influences were discarded and previously
            // compounded the lossy 5% cutoff that used to follow.
            std::vector<std::vector<BoneInfluence>> vertexInfluences(meshData.vertices.size());

            for (unsigned int i = 0; i < assimpMesh->mNumBones; i++) {
                const aiBone* assimpBone = assimpMesh->mBones[i];
                if (!BoneHasRelevantWeight(assimpBone)) {
                    continue;
                }

                const std::string boneName = assimpBone->mName.C_Str();
                const auto mappedBone = modelData.boneMapping.find(boneName);
                if (mappedBone == modelData.boneMapping.end()) {
                    std::cout << "ImportSkinnedModel() found weights for pruned bone '" << boneName << "' in '" << filepath << "'\n";
                    importer.FreeScene();
                    return {};
                }
                const unsigned int boneIndex = mappedBone->second;

                for (unsigned int j = 0; j < assimpBone->mNumWeights; j++) {
                    const unsigned int vertexIndex = assimpBone->mWeights[j].mVertexId;
                    const float weight = assimpBone->mWeights[j].mWeight;

                    if (vertexIndex >= vertexInfluences.size()) {
                        std::cout << "ImportSkinnedModel() found an out-of-range vertex weight in '" << filepath << "'\n";
                        importer.FreeScene();
                        return {};
                    }

                    if (weight > BONE_WEIGHT_EPSILON) {
                        vertexInfluences[vertexIndex].push_back({ boneIndex, weight });
                    }
                }
            }

            // Keep the strongest four influences and preserve every meaningful
            // source weight, including the small neck/shoulder blends used by
            // hair cards. Renormalize only because the GPU format has four slots.
            for (std::size_t vertexIndex = 0; vertexIndex < vertexInfluences.size(); vertexIndex++) {
                std::vector<BoneInfluence>& influences = vertexInfluences[vertexIndex];
                std::sort(influences.begin(), influences.end(), [](const BoneInfluence& a, const BoneInfluence& b) {
                    if (a.weight != b.weight) return a.weight > b.weight;
                    return a.boneIndex < b.boneIndex;
                });

                const std::size_t influenceCount = std::min(influences.size(), MAX_BONE_INFLUENCES);
                float weightSum = 0.0f;
                for (std::size_t influenceIndex = 0; influenceIndex < influenceCount; influenceIndex++) {
                    weightSum += influences[influenceIndex].weight;
                }

                if (weightSum <= BONE_WEIGHT_EPSILON) {
                    std::cout << "ImportSkinnedModel() found an unweighted vertex " << vertexIndex << " in mesh '" << meshData.name << "' from '" << filepath << "'\n";
                    importer.FreeScene();
                    return {};
                }

                VertexWeight& vertexWeight = meshData.vertexWeights[vertexIndex];
                for (std::size_t influenceIndex = 0; influenceIndex < influenceCount; influenceIndex++) {
                    vertexWeight.boneID[influenceIndex] = static_cast<int>(influences[influenceIndex].boneIndex);
                    vertexWeight.weight[influenceIndex] = influences[influenceIndex].weight / weightSum;
                }
            }

            // Check if all vertices have only one weight
            bool allVerticesHaveOnlyOneWeight = !meshData.vertexWeights.empty();

            for (const VertexWeight& vertexWeight : meshData.vertexWeights) {
                if (vertexWeight.weight.y > 0.0f ||
                    vertexWeight.weight.z > 0.0f ||
                    vertexWeight.weight.w > 0.0f) {
                    allVerticesHaveOnlyOneWeight = false;
                    break;
                }
            }

            // If they do, now check they all reference the same bone
            const int foundBoneIndex = allVerticesHaveOnlyOneWeight ? meshData.vertexWeights[0].boneID[0] : -1;
            bool allVerticesAlsoOnlyReferenceTheSameBone = allVerticesHaveOnlyOneWeight;

            if (allVerticesHaveOnlyOneWeight) {
                for (std::size_t i = 1; i < meshData.vertexWeights.size(); i++) {
                    const VertexWeight& vertexWeight = meshData.vertexWeights[i];
                    if (vertexWeight.boneID.x != foundBoneIndex) {
                        allVerticesAlsoOnlyReferenceTheSameBone = false;
                        break;
                    }
                }
            }

            // SET THE BOOLEAN
            if (allVerticesHaveOnlyOneWeight && allVerticesAlsoOnlyReferenceTheSameBone) {
                meshData.requiresSkinning = false;
                meshData.nonDeformingBoneIndex = foundBoneIndex;
            }
            else {
                meshData.requiresSkinning = true;
                meshData.nonDeformingBoneIndex = -1;
            }

            std::cout << modelData.name << " [" << meshData.name << "]: " << Hell::String::FormatBool(meshData.requiresSkinning) << " " << foundBoneIndex << " nonDeformingBoneIndex " << meshData.vertexCount << " verts \n";

            modelData.vertexCount += (uint32_t)meshData.vertices.size();
            modelData.indexCount += (uint32_t)meshData.indices.size();
        }

        for (const std::string& requestedMorphTarget : requestedMorphTargets) {
            if (!importedMorphTargetNames.contains(requestedMorphTarget)) {
                std::cout << "ImportSkinnedModel() did not find requested morph target '"
                          << requestedMorphTarget << "' in '" << filepath << "'\n";
            }
        }

        if (!requestedMorphTargets.empty()) {
            std::cout << "ImportSkinnedModel() stored " << importedMorphTargetCount
                      << " sparse morph targets with " << importedMorphPositionDeltaCount << " position, "
                      << importedMorphNormalDeltaCount << " normal, and " << importedMorphTangentDeltaCount
                      << " tangent deltas from '" << filepath << "'\n";
        }

        // Cleanup
        importer.FreeScene();

        return modelData;
    }
}
