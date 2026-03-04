#include "GlobalIllumination.h"
#include "AssetManagement/AssetManager.h"
#include "Bvh/Gpu/Bvh.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "Physics/Physics.h"
#include "World/World.h"

#include "Core/Game.h" 

namespace GlobalIllumination {

    struct Triangle {
        glm::vec3 v0;
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 normal;
    };

    void CreatePointCloud();
    void CreateDoorBvh();
    void CreateHouseBvh();
    void UpdateSceneBVh();

    void InitPointGrid(); // Rename me and put me somewhere better

    inline float RoundUp(float value, float spacing)    { return std::ceil(value / spacing) * spacing; }
    inline float RoundDown(float value, float spacing)  { return std::floor(value / spacing) * spacing; }

    struct GlobalIlluminationState {
        float probeSpacing = 0.75f;

        uint64_t houseBvhId = 0;
        uint64_t doorBvhId = 0;
        uint64_t sceneBvhId = 0;

        glm::vec3 houseMinBounds = glm::vec3(0.0f);
        glm::vec3 houseMaxBounds = glm::vec3(0.0f);

        std::vector<LightVolume> lightVolumes;
        std::vector<Triangle> triangles;
        std::vector<CloudPoint> pointCloud;
        bool globalIlluminationStructuresDirty = false;
        bool pointCloudNeedsGpuUpdate = false;

        std::vector<PointCloudOctrant> pointCloudOctrants;
        std::vector<unsigned int> pointIndices;
        glm::uvec3 pointCloudGridDimensions = glm::uvec3(0);
        glm::vec3 pointGridWorldMin = glm::vec3(0.0f);
        glm::vec3 pointGridWorldMax = glm::vec3(0.0f);
        glm::vec3 pointCloudOctrantSize = glm::vec3(0.0f);

        static constexpr float pointCloudOctrantSpacing = 5.0f;
    };

    GlobalIlluminationState& GetState() {
        static GlobalIlluminationState state;
        return state;
    }

    void Update() {
        if (GetState().globalIlluminationStructuresDirty) {
            GlobalIllumination::CreatePointCloud();
            GlobalIllumination::CreateHouseBvh();
            if (GetState().doorBvhId == 0) {
                CreateDoorBvh();
            }
            if (GetState().sceneBvhId == 0) {
                GetState().sceneBvhId = Bvh::Gpu::CreateNewSceneBvh();
            }
            Bvh::Gpu::FlatternMeshBvhNodes();
            GetState().globalIlluminationStructuresDirty = false;
        }

        UpdateSceneBVh();
    }

    void CreatePointCloud() {
        GetState().triangles.clear();

        // Store floor and ceilings triangles
        for (HousePlane& plane : World::GetHousePlanes()) {
            for (uint32_t i = 0; i < plane.GetIndices().size(); i+=3) {
                Triangle& triangle = GetState().triangles.emplace_back();

                int idx0 = plane.GetIndices()[i + 0];
                int idx1 = plane.GetIndices()[i + 1];
                int idx2 = plane.GetIndices()[i + 2];

                triangle.v0 = plane.GetVertices()[idx0].position;
                triangle.v1 = plane.GetVertices()[idx1].position;
                triangle.v2 = plane.GetVertices()[idx2].position;

                triangle.normal = normalize(
                    plane.GetVertices()[idx0].normal +
                    plane.GetVertices()[idx1].normal +
                    plane.GetVertices()[idx2].normal
                );
            }
        }

        // Store wall triangles
        for (Wall& wall : World::GetWalls()) {
            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                for (uint32_t i = 0; i < wallSegment.GetIndices().size(); i += 3) {
                    Triangle& triangle = GetState().triangles.emplace_back();
                    
                    int idx0 = wallSegment.GetIndices()[i + 0];
                    int idx1 = wallSegment.GetIndices()[i + 1];
                    int idx2 = wallSegment.GetIndices()[i + 2];
                    
                    triangle.v0 = wallSegment.GetVertices()[idx0].position;
                    triangle.v1 = wallSegment.GetVertices()[idx1].position;
                    triangle.v2 = wallSegment.GetVertices()[idx2].position;
                    
                    triangle.normal = normalize(
                        wallSegment.GetVertices()[idx0].normal +
                        wallSegment.GetVertices()[idx1].normal +
                        wallSegment.GetVertices()[idx2].normal
                    );
                }
            }
        }

        GetState().pointCloud.clear();

        for (Triangle& triangle : GetState().triangles) {

            // Make sure normal is valid
            glm::vec3 edge1 = triangle.v1 - triangle.v0;
            glm::vec3 edge2 = triangle.v2 - triangle.v0;
            triangle.normal = glm::cross(edge1, edge2);
            triangle.normal = glm::normalize(triangle.normal);

            // Calculate the normal of the triangle
            const glm::vec3& normal = triangle.normal;

            // Choose the up vector based on the normal
            glm::vec3 up = (std::abs(normal.z) > 0.999f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

            // Calculate right and up vectors
            glm::vec3 right = glm::normalize(glm::cross(up, normal));
            up = glm::cross(normal, right);  // No need to normalize again as the cross product of two unit vectors is already normalized

            glm::mat3 transform(right.x, right.y, right.z,up.x, up.y, up.z,normal.x, normal.y, normal.z);

            glm::vec2 v0_2d(glm::dot(right, triangle.v0), glm::dot(up, triangle.v0));
            glm::vec2 v1_2d(glm::dot(right, triangle.v1), glm::dot(up, triangle.v1));
            glm::vec2 v2_2d(glm::dot(right, triangle.v2), glm::dot(up, triangle.v2));

            // Determine the bounding box of the 2D triangle
            glm::vec2 min = glm::min(glm::min(v0_2d, v1_2d), v2_2d);
            glm::vec2 max = glm::max(glm::max(v0_2d, v1_2d), v2_2d);

            // Round min and max values
            min.x = RoundDown(min.x, POINT_CLOUD_SPACING) - POINT_CLOUD_SPACING * 0.5f;
            min.y = RoundDown(min.y, POINT_CLOUD_SPACING) - POINT_CLOUD_SPACING * 0.5f;
            max.x = RoundUp(max.x, POINT_CLOUD_SPACING) + POINT_CLOUD_SPACING * 0.5f;
            max.y = RoundUp(max.y, POINT_CLOUD_SPACING) + POINT_CLOUD_SPACING * 0.5f;

            float threshold = 0.05f;
            min.x += threshold;
            min.y += threshold;
            max.x -= threshold;
            max.y -= threshold;

            // Generate points within the bounding box
            for (float x = min.x; x <= max.x; x += POINT_CLOUD_SPACING) {
                for (float y = min.y; y <= max.y; y += POINT_CLOUD_SPACING) {
                    glm::vec2 pt(x, y);
                    if (Util::IsPointInTriangle2D(pt, v0_2d, v1_2d, v2_2d)) {
                        glm::vec3 pt3d = triangle.v0 + right * (pt.x - v0_2d.x) + up * (pt.y - v0_2d.y);

                        CloudPoint& cloudPoint = GetState().pointCloud.emplace_back();
                        cloudPoint.position = glm::vec4(pt3d, 0.0f);
                        cloudPoint.normal = glm::vec4(triangle.normal, 0.0f);
                    }
                }
            }
        }

        GetState().pointCloudNeedsGpuUpdate = true;

        std::cout << "Recreated point cloud: " << GetState().pointCloud.size() << " points \n";
    }

    void CreateHouseBvh() {     
        // Destroy any previous house bvh
        if (GetState().houseBvhId != 0) {
            Bvh::Gpu::DestroyMeshBvh(GetState().houseBvhId);
        }

        GetState().houseMinBounds = glm::vec3(std::numeric_limits<float>::max());
        GetState().houseMaxBounds = glm::vec3(-std::numeric_limits<float>::max());

        // Create house vertices
        std::vector<Vertex> vertices;
        for (Triangle& triangle : GetState().triangles) {
            Vertex v0, v1, v2;
            v0.position = triangle.v0;
            v1.position = triangle.v1;
            v2.position = triangle.v2;
            v0.normal = triangle.normal;
            v1.normal = triangle.normal;
            v2.normal = triangle.normal;
            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);

            GetState().houseMinBounds.x = std::min(GetState().houseMinBounds.x, v0.position.x);
            GetState().houseMinBounds.y = std::min(GetState().houseMinBounds.y, v0.position.y);
            GetState().houseMinBounds.z = std::min(GetState().houseMinBounds.z, v0.position.z);
            GetState().houseMaxBounds.x = std::max(GetState().houseMaxBounds.x, v0.position.x);
            GetState().houseMaxBounds.y = std::max(GetState().houseMaxBounds.y, v0.position.y);
            GetState().houseMaxBounds.z = std::max(GetState().houseMaxBounds.z, v0.position.z);

            GetState().houseMinBounds.x = std::min(GetState().houseMinBounds.x, v1.position.x);
            GetState().houseMinBounds.y = std::min(GetState().houseMinBounds.y, v1.position.y);
            GetState().houseMinBounds.z = std::min(GetState().houseMinBounds.z, v1.position.z);
            GetState().houseMaxBounds.x = std::max(GetState().houseMaxBounds.x, v1.position.x);
            GetState().houseMaxBounds.y = std::max(GetState().houseMaxBounds.y, v1.position.y);
            GetState().houseMaxBounds.z = std::max(GetState().houseMaxBounds.z, v1.position.z);

            GetState().houseMinBounds.x = std::min(GetState().houseMinBounds.x, v2.position.x);
            GetState().houseMinBounds.y = std::min(GetState().houseMinBounds.y, v2.position.y);
            GetState().houseMinBounds.z = std::min(GetState().houseMinBounds.z, v2.position.z);
            GetState().houseMaxBounds.x = std::max(GetState().houseMaxBounds.x, v2.position.x);
            GetState().houseMaxBounds.y = std::max(GetState().houseMaxBounds.y, v2.position.y);
            GetState().houseMaxBounds.z = std::max(GetState().houseMaxBounds.z, v2.position.z);
        }

        // Create house indices
        std::vector<uint32_t> indices(vertices.size());
        for (size_t i = 0; i < vertices.size(); i++) {
            indices[i] = static_cast<uint32_t>(i);
        }

        GetState().houseBvhId = Bvh::Gpu::CreateMeshBvhFromVertexData(vertices, indices);

        // For now you only have one light volume, for whatever house you just made.
        for (LightVolume& lightVolume : GetState().lightVolumes) {
            lightVolume.CleanUp();
        }
        GetState().lightVolumes.clear();
        LightVolume& lightVolume = GetState().lightVolumes.emplace_back();
        lightVolume.Init(vertices, GetState().houseMinBounds, GetState().houseMaxBounds);

        InitPointGrid();
    }

    void CreateDoorBvh() {
        Mesh* cubeMesh = AssetManager::GetCubeMesh();
        std::vector<Vertex> vertices = AssetManager::GetMeshVertices(cubeMesh);

        for (Vertex& vertex : vertices) {
            vertex.position *= glm::vec3(DOOR_DEPTH, DOOR_HEIGHT, DOOR_WIDTH);
            vertex.position.x += -0.005f;
            vertex.position.z -= DOOR_WIDTH / 2;
            vertex.position.y += DOOR_HEIGHT / 2;
            vertex.position.x -= DOOR_DEPTH / 2;
        }

        std::vector<uint32_t> indices(vertices.size());
        for (int i = 0; i < vertices.size(); i++) {
            indices[i] = i;
        }

        GetState().doorBvhId = Bvh::Gpu::CreateMeshBvhFromVertexData(vertices, indices);
    }
    
    void UpdateSceneBVh() {
        if (GetState().sceneBvhId == 0 || GetState().houseBvhId == 0) {
            return;
        }

        std::vector<PrimitiveInstance> instances;
        instances.reserve(1 + World::GetDoors().size());

        // Add the house
        PrimitiveInstance& instance = instances.emplace_back();
        instance.worldAabbBoundsMin.x = GetState().houseMinBounds.x;
        instance.worldAabbBoundsMin.y = GetState().houseMinBounds.y;
        instance.worldAabbBoundsMin.z = GetState().houseMinBounds.z;
        instance.worldAabbBoundsMax.x = GetState().houseMaxBounds.x;
        instance.worldAabbBoundsMax.y = GetState().houseMaxBounds.y;
        instance.worldAabbBoundsMax.z = GetState().houseMaxBounds.z;
        instance.objectId = 0;
        instance.worldTransform = glm::mat4(1.0f);
        instance.meshBvhId = GetState().houseBvhId;
        instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;

        // Add all the doors
        for (Door& door : World::GetDoors()) {
            (void)door;
            // BROKKEN BECAUSE OF physicsId now gone
            //uint64_t rigidStaticId = door.GetPhysicsId();
            //RigidStatic* rigidStatic = Physics::GetRigidStaitcById(rigidStaticId);
            //PxRigidStatic* pxRigidStatic = rigidStatic->GetPxRigidStatic();
            //PxRigidActor* pxRigidActor = static_cast<PxRigidActor*>(pxRigidStatic);
            //PxBounds3 bounds = pxRigidActor->getWorldBounds();
            //PxVec3 minBounds = bounds.minimum;
            //PxVec3 maxBounds = bounds.maximum;
            //
            //PrimitiveInstance& instance = instances.emplace_back();
            //instance.worldAabbBoundsMin.x = minBounds.x;
            //instance.worldAabbBoundsMin.y = minBounds.y;
            //instance.worldAabbBoundsMin.z = minBounds.z;
            //instance.worldAabbBoundsMax.x = maxBounds.x;
            //instance.worldAabbBoundsMax.y = maxBounds.y;
            //instance.worldAabbBoundsMax.z = maxBounds.z;
            //instance.objectId = door.GetObjectId();
            //instance.worldTransform = door.GetDoorModelMatrix();
            //instance.meshBvhId = GetState().doorBvhId;
            //instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;
        }

        Bvh::Gpu::UpdateSceneBvh(GetState().sceneBvhId, instances);
    }

    void InitPointGrid() {

        for (LightVolume& lightVolume : GetState().lightVolumes) {

            GetState().pointGridWorldMin = lightVolume.m_offset;
            GetState().pointGridWorldMax = lightVolume.m_offset + glm::vec3(lightVolume.m_worldSpaceWidth, lightVolume.m_worldSpaceHeight, lightVolume.m_worldSpaceDepth);

            glm::vec3 worldSize = GetState().pointGridWorldMax - GetState().pointGridWorldMin;
            GetState().pointCloudGridDimensions.x = static_cast<unsigned int>(glm::ceil(worldSize.x / GlobalIlluminationState::pointCloudOctrantSpacing));
            GetState().pointCloudGridDimensions.y = static_cast<unsigned int>(glm::ceil(worldSize.y / GlobalIlluminationState::pointCloudOctrantSpacing));
            GetState().pointCloudGridDimensions.z = static_cast<unsigned int>(glm::ceil(worldSize.z / GlobalIlluminationState::pointCloudOctrantSpacing));

            GetState().pointCloudOctrantSize = worldSize / glm::vec3(GetState().pointCloudGridDimensions);

             // Bail if point cloud is empty
            if (GetState().pointCloud.empty()) return;

            // For each grid cell, count how many points fall inside it
            unsigned int totalCells = GetState().pointCloudGridDimensions.x * GetState().pointCloudGridDimensions.y * GetState().pointCloudGridDimensions.z;
            std::vector<unsigned int> cellCounts(totalCells, 0);

            for (const auto& point : GetState().pointCloud) {
                // Find out which grid cell this point belongs to
                glm::vec3 relativePos = glm::vec3(point.position) - GetState().pointGridWorldMin;
                glm::ivec3 cellCoords = glm::ivec3(relativePos / GetState().pointCloudOctrantSize);

                // Make sure the coordinates are within the grid bounds, just in case
                cellCoords = glm::clamp(cellCoords, glm::ivec3(0), glm::ivec3(GetState().pointCloudGridDimensions) - 1);

                // Convert the 3D cell coordinate into a 1D array index and increment the counter
                unsigned int cellIndex = (cellCoords.z * GetState().pointCloudGridDimensions.x * GetState().pointCloudGridDimensions.y) + (cellCoords.y * GetState().pointCloudGridDimensions.x) + cellCoords.x;
                cellCounts[cellIndex]++;
            }

            // Create the final PointGridCell structures with the correct offsets
            GetState().pointCloudOctrants.resize(totalCells);
            unsigned int currentOffset = 0;
            for (unsigned int i = 0; i < totalCells; ++i) {
                GetState().pointCloudOctrants[i].m_cloudPointCount = cellCounts[i];
                GetState().pointCloudOctrants[i].m_offset = currentOffset;
                currentOffset += cellCounts[i]; // The next cell's offset starts after all of this cell's points
            }

            // Finally, we create the master list of sorted point indices
            GetState().pointIndices.resize(GetState().pointCloud.size());
            std::vector<unsigned int> tempOffsets(totalCells);
            for (unsigned int i = 0; i < totalCells; ++i) {
                tempOffsets[i] = GetState().pointCloudOctrants[i].m_offset;
            }

            // Go through the original points again...
            for (unsigned int i = 0; i < GetState().pointCloud.size(); ++i) {
                const auto& point = GetState().pointCloud[i];

                // Find which cell it belongs to...
                glm::vec3 relativePos = glm::vec3(point.position) - GetState().pointGridWorldMin;
                glm::ivec3 cellCoords = glm::ivec3(relativePos / GetState().pointCloudOctrantSize);
                cellCoords = glm::clamp(cellCoords, glm::ivec3(0), glm::ivec3(GetState().pointCloudGridDimensions) - 1);
                unsigned int cellIndex = (cellCoords.z * GetState().pointCloudGridDimensions.x * GetState().pointCloudGridDimensions.y) + (cellCoords.y * GetState().pointCloudGridDimensions.x) + cellCoords.x;

                // Use the write counter to place the point's original index i in the correct slot
                unsigned int& insertionIndex = tempOffsets[cellIndex];
                GetState().pointIndices[insertionIndex] = i;

                // Increment the write counter for that cell
                insertionIndex++;
            }
        }
        std::cout << "Point cloud octrant grid created\n";
        std::cout << "pointIndices:       " << GetState().pointIndices.size() << "\n";
        std::cout << "pointCloudOctrants: " << GetState().pointCloudOctrants.size() << "\n";
        
    }

    uint64_t GetSceneBvhId() {
        return GetState().sceneBvhId;
    }

    const std::vector<BvhNode>& GetSceneNodes() {
        static std::vector<BvhNode> empty;
        if (GetState().sceneBvhId == 0) {
            return empty;
        }

        SceneBvh* sceneBvh = Bvh::Gpu::GetSceneBvhById(GetState().sceneBvhId);
        if (!sceneBvh) return empty;

        return sceneBvh->m_nodes;
    }

    std::vector<CloudPoint>& GetPointCloud() {
        return GetState().pointCloud;
    }

    std::vector<LightVolume>& GetLightVolumes() {
        return GetState().lightVolumes;
    }

    void SetGlobalIlluminationStructuresDirtyState(bool state) {
        GetState().globalIlluminationStructuresDirty = state;
    }

    bool GlobalIlluminationStructuresAreDirty() {
        return GetState().globalIlluminationStructuresDirty;
    }

    void SetPointCloudNeedsGpuUpdateState(bool state) {
        GetState().pointCloudNeedsGpuUpdate = state;
    }

    bool PointCloudNeedsGpuUpdate() {
        return GetState().pointCloudNeedsGpuUpdate;
    }

    float GetProbeSpacing() {
        return GetState().probeSpacing;
    }

    std::vector<PointCloudOctrant>& GetPointCloudOctrants() {
        return GetState().pointCloudOctrants;
    }

    std::vector<unsigned int>& GetPointIndices() {
        return GetState().pointIndices;
    }

    glm::uvec3 GetPointCloudGridDimensions() {
        return GetState().pointCloudGridDimensions;
    }

    glm::vec3 GetPointGridWorldMin() {
        return GetState().pointGridWorldMin;
    }

    glm::vec3 GetPointGridWorldMax() {
        return GetState().pointGridWorldMax;
    }
}

void LightVolume::Init(const std::vector<Vertex>& vertices, const glm::vec3& aabbMin, const glm::vec3& aabbMax) {
    glm::vec3 inflatedAabbMin = aabbMin - glm::vec3(1.0f);
    glm::vec3 inflatedAabbMax = aabbMax + glm::vec3(2.0f);
    m_offset = inflatedAabbMin;
    m_worldSpaceWidth = inflatedAabbMax.x - inflatedAabbMin.x;
    m_worldSpaceHeight = inflatedAabbMax.y - inflatedAabbMin.y;
    m_worldSpaceDepth = inflatedAabbMax.z - inflatedAabbMin.z;
    m_textureWidth = int(m_worldSpaceWidth / GlobalIllumination::GetProbeSpacing());
    m_textureHeight = int(m_worldSpaceHeight / GlobalIllumination::GetProbeSpacing());
    m_textureDepth = int(m_worldSpaceDepth / GlobalIllumination::GetProbeSpacing());

    // Create the 3d textures
    glGenTextures(1, &m_lightVolumeA);
    glBindTexture(GL_TEXTURE_3D, m_lightVolumeA);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, m_textureWidth, m_textureHeight, m_textureDepth, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_lightVolumeB);
    glBindTexture(GL_TEXTURE_3D, m_lightVolumeB);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, m_textureWidth, m_textureHeight, m_textureDepth, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    m_lightVolumeTextures[0] = m_lightVolumeA;
    m_lightVolumeTextures[1] = m_lightVolumeB;

    glGenTextures(1, &m_lightVolumeMaskTexture);
    glBindTexture(GL_TEXTURE_3D, m_lightVolumeMaskTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, m_textureWidth, m_textureHeight, m_textureDepth, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);
}

void LightVolume::CleanUp() {
    glDeleteTextures(1, &m_lightVolumeA);
    glDeleteTextures(1, &m_lightVolumeB);
    glDeleteTextures(1, &m_lightVolumeMaskTexture);
}

GLuint LightVolume::GetLightingTextureHandle() {
    return m_lightVolumeTextures[m_pingPongReadIndex];
}

GLuint LightVolume::GetMaskTextureHandle() {
    return m_lightVolumeMaskTexture;
}
