#include "AssimpImporter.h"
#include "File.h"
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>
#include <assimp/Scene.h>
#include <assimp/PostProcess.h>
#include "../Util/Util.h"
#include <numeric>


#include <map>
#include <unordered_map>
#include <string>

namespace AssimpImporter {

    ModelData ImportFbx(const std::string& filepath) {
        ModelData modelData;
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
        const aiScene* scene = importer.ReadFile(filepath,
                                                 aiProcess_Triangulate |
                                                 aiProcess_JoinIdenticalVertices |
                                                 aiProcess_ImproveCacheLocality |
                                                 aiProcess_RemoveRedundantMaterials |
                                                 aiProcess_FlipUVs
        );
        if (!scene) {
            std::cout << "LoadAndExportCustomFormat() failed to loaded model " << filepath << "\n";
            std::cerr << "Assimp Error: " << importer.GetErrorString() << "\n";
            return modelData;
        }
        modelData.name = Util::GetFileName(filepath);
        modelData.meshCount = scene->mNumMeshes;
        modelData.meshes.resize(modelData.meshCount);
        modelData.timestamp = File::GetLastModifiedTime(filepath);

        // Pre allocate vector memory
        for (int i = 0; i < modelData.meshes.size(); i++) {
            MeshData& meshData = modelData.meshes[i];
            meshData.vertexCount = scene->mMeshes[i]->mNumVertices;
            meshData.indexCount = scene->mMeshes[i]->mNumFaces * 3;
            meshData.name = scene->mMeshes[i]->mName.C_Str();
            meshData.vertices.resize(meshData.vertexCount);
            meshData.indices.resize(meshData.indexCount);
            // Remove blender naming mess
            meshData.name = meshData.name.substr(0, meshData.name.find('.'));
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
                meshData.aabbMin = Util::Vec3Min(meshData.vertices[j].position, meshData.aabbMin);
                meshData.aabbMax = Util::Vec3Max(meshData.vertices[j].position, meshData.aabbMax);
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
            // Generate Tangents
            for (int i = 0; i < meshData.indices.size(); i += 3) {
                Vertex* vert0 = &meshData.vertices[meshData.indices[i]];
                Vertex* vert1 = &meshData.vertices[meshData.indices[i + 1]];
                Vertex* vert2 = &meshData.vertices[meshData.indices[i + 2]];
                glm::vec3 deltaPos1 = vert1->position - vert0->position;
                glm::vec3 deltaPos2 = vert2->position - vert0->position;
                glm::vec2 deltaUV1 = vert1->uv - vert0->uv;
                glm::vec2 deltaUV2 = vert2->uv - vert0->uv;
                float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
                glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
                glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
                vert0->tangent = tangent;
                vert1->tangent = tangent;
                vert2->tangent = tangent;
            }
            modelData.aabbMin = Util::Vec3Min(modelData.aabbMin, meshData.aabbMin);
            modelData.aabbMax = Util::Vec3Max(modelData.aabbMax, meshData.aabbMax);
        }
        importer.FreeScene();
        return modelData;
    }

    void GrabSkeleton(std::vector<Node>& nodes, const aiNode* pNode, int parentIndex) {
        // Create the joint node
        Node node;
        node.name = pNode->mName.C_Str();
        node.inverseBindTransform = Util::aiMatrix4x4ToGlm(pNode->mTransformation);
        node.parentIndex = parentIndex;

        // Determine the current node's index and push it
        int currentIndex = static_cast<int>(nodes.size());
        nodes.push_back(node);

        // Recursively process children using the current node's index as parentIndex
        for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
            GrabSkeleton(nodes, pNode->mChildren[i], currentIndex);
        }
    }

    SkinnedModelData ImportSkinnedFbx(const std::string& filepath) {
        SkinnedModelData modelData;

        unsigned int flags = 
            aiProcess_LimitBoneWeights |
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace;

        if (Util::GetFileInfoFromPath(filepath).name == "Knife" ||
            Util::GetFileInfoFromPath(filepath).name == "Tokarev" ||
            Util::GetFileInfoFromPath(filepath).name == "GoldenGlock") {
            flags = 
                aiProcess_LimitBoneWeights |
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace |
                aiProcess_GlobalScale; // Knife uses this also
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath.c_str(), flags);

        if (!scene) {
            std::cout << "Something fucked up loading your skinned model: " << filepath << "\n";
            std::cout << "Error: " << importer.GetErrorString() << "\n";
            return modelData;
        }

        modelData.name = Util::GetFileName(filepath);
        modelData.meshes.resize(scene->mNumMeshes);
        modelData.timestamp = File::GetLastModifiedTime(filepath);
        modelData.vertexCount = 0;
        modelData.indexCount = 0;

        // Load bones
        int foundBoneCount = 0;
        for (int i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* assimpMesh = scene->mMeshes[i];

            for (unsigned int j = 0; j < assimpMesh->mNumBones; j++) {
                const aiBone* bone = assimpMesh->mBones[j];
                std::string boneName = bone->mName.data;

                // If this bone isn't mapped yet, add it to the global list
                bool inserted = modelData.boneMapping.find(boneName) != modelData.boneMapping.end();
                if (!inserted) {

                    // Map bone name to index
                    unsigned int boneIndex = foundBoneCount++;
                    modelData.boneMapping[boneName] = boneIndex;

                    // Store bone info
                    glm::mat4 boneOffset = Util::aiMatrix4x4ToGlm(bone->mOffsetMatrix);
                    modelData.boneOffsets.push_back(boneOffset);

                    //if (modelData.name == "Knife") {
                    //    std::cout << boneName << ": " << boneIndex << "\n";
                    //}
                }
            }
        }

        // Recursively grab the skeleton
        GrabSkeleton(modelData.nodes, scene->mRootNode, -1);

        // Get vertex data
        uint32_t localBaseVertex = 0;
        for (int i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* assimpMesh = scene->mMeshes[i];

            SkinnedMeshData& meshData = modelData.meshes[i];;
            meshData.aabbMin = glm::vec3(std::numeric_limits<float>::max());
            meshData.aabbMax = glm::vec3(-std::numeric_limits<float>::max());
            meshData.vertexCount = assimpMesh->mNumVertices;
            meshData.indexCount = assimpMesh->mNumFaces * 3;
            meshData.name = assimpMesh->mName.C_Str();
            meshData.localBaseVertex = localBaseVertex;
            meshData.vertices.reserve(meshData.vertexCount);
            meshData.indices.reserve(meshData.indexCount);

            // Get vertices
            for (unsigned int j = 0; j < meshData.vertexCount; j++) {
                WeightedVertex vertex;
                vertex.position = { assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z };
                vertex.normal = { assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z };
                vertex.tangent = { assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z };
                vertex.uv = { assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f) };
                meshData.vertices.push_back(vertex);
                meshData.aabbMin.x = std::min(meshData.aabbMin.x, vertex.position.x);
                meshData.aabbMin.y = std::min(meshData.aabbMin.y, vertex.position.y);
                meshData.aabbMin.z = std::min(meshData.aabbMin.z, vertex.position.z);
                meshData.aabbMax.x = std::max(meshData.aabbMax.x, vertex.position.x);
                meshData.aabbMax.y = std::max(meshData.aabbMax.y, vertex.position.y);
                meshData.aabbMax.z = std::max(meshData.aabbMax.z, vertex.position.z);
            }
            // Get indices
            for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
                const aiFace& Face = assimpMesh->mFaces[j];
                meshData.indices.push_back(Face.mIndices[0]);
                meshData.indices.push_back(Face.mIndices[1]);
                meshData.indices.push_back(Face.mIndices[2]);
            }

            // Get vertex weights and bone IDs
            std::vector<unsigned int> influenceCount(meshData.vertices.size(), 0);

            for (unsigned int i = 0; i < assimpMesh->mNumBones; i++) {
                std::string boneName = assimpMesh->mBones[i]->mName.data;
                unsigned int boneIndex = modelData.boneMapping[boneName];

                for (unsigned int j = 0; j < assimpMesh->mBones[i]->mNumWeights; j++) {
                    unsigned int vertexIndex = assimpMesh->mBones[i]->mWeights[j].mVertexId;
                    float weight = assimpMesh->mBones[i]->mWeights[j].mWeight;
                    WeightedVertex& vertex = meshData.vertices[vertexIndex];

                    if (influenceCount[vertexIndex] < 4) {
                        switch (influenceCount[vertexIndex]) {
                            case 0:
                            vertex.boneID.x = boneIndex;
                            vertex.weight.x = weight;
                            break;
                            case 1:
                            vertex.boneID.y = boneIndex;
                            vertex.weight.y = weight;
                            break;
                            case 2:
                            vertex.boneID.z = boneIndex;
                            vertex.weight.z = weight;
                            break;
                            case 3:
                            vertex.boneID.w = boneIndex;
                            vertex.weight.w = weight;
                            break;
                        }
                        influenceCount[vertexIndex]++;
                    }
                }
            }

            // Ingore broken weights
            float threshold = 0.05f;
            for (unsigned int j = 0; j < meshData.vertices.size(); j++) {
                WeightedVertex& vertex = meshData.vertices[j];
                std::vector<float> validWeights;
                for (int i = 0; i < 4; ++i) {
                    if (vertex.weight[i] < threshold) {
                        vertex.weight[i] = 0.0f;
                    }
                    else {
                        validWeights.push_back(vertex.weight[i]);
                    }
                }
                float sum = std::accumulate(validWeights.begin(), validWeights.end(), 0.0f);
                int validIndex = 0;
                for (int i = 0; i < 4; ++i) {
                    if (vertex.weight[i] > 0.0f) {
                        vertex.weight[i] = validWeights[validIndex] / sum;
                        validIndex++;
                    }
                }
            }


            // Check if all vertices have only one weight
            bool allVerticeHaveOnlyOneWeight = true;

            for (WeightedVertex& vertex : meshData.vertices) {
                if (vertex.weight.y != 0 ||
                    vertex.weight.z != 0 ||
                    vertex.weight.w != 0) {
                    allVerticeHaveOnlyOneWeight = false;
                    break;
                }
            }

            // If they do, now check they all reference the same bone
            int foundBoneIndex = meshData.vertices.empty() ? -1 : meshData.vertices[0].boneID[0];
            bool allVerticesAlsoOnlyReferenceTheSameBone = true;

            if (allVerticeHaveOnlyOneWeight) {
                for (int i = 1; i < meshData.vertices.size(); i++) {
                    WeightedVertex& vertex = meshData.vertices[i];
                    if (vertex.boneID.x != foundBoneIndex) {
                        allVerticesAlsoOnlyReferenceTheSameBone = false;
                        break;
                    }
                }
            }

            // SET THE BOOLEAN
            if (allVerticeHaveOnlyOneWeight && allVerticesAlsoOnlyReferenceTheSameBone) {
                meshData.requiresSkinning = false;
                meshData.nonDeformingBoneIndex = foundBoneIndex;
            }
            else {
                meshData.requiresSkinning = true;
                meshData.nonDeformingBoneIndex = -1;
            }

            std::cout << modelData.name << " [" << meshData.name << "]: " << Util::BoolToString(meshData.requiresSkinning) << " " << foundBoneIndex << " nonDeformingBoneIndex " << meshData.vertexCount << " verts \n";

            localBaseVertex += (uint32_t)meshData.vertices.size();
            modelData.vertexCount += (uint32_t)meshData.vertices.size();
            modelData.indexCount += (uint32_t)meshData.indices.size();
        }

        // Cleanup
        importer.FreeScene();

        return modelData;
    }
}
