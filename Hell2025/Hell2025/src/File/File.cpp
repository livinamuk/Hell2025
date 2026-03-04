#include "File.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <chrono>
#include <cstring> // For std::memset, std::memcpy
#include <string>
#include <cstddef> // For size_t
#include "Util.h"

#define PRINT_SKINNED_MODEL_HEADERS_ON_READ 0
#define PRINT_SKINNED_MODEL_HEADERS_ON_WRITE 0
#define PRINT_SKINNED_MESH_HEADERS_ON_READ 0
#define PRINT_SKINNED_MESH_HEADERS_ON_WRITE 0

namespace File {

    void MemCopyName(char* nameBuffer, const std::string& name) {
        std::memset(nameBuffer, 0, HELL_NAME_BUFFER_SIZE);
        std::memcpy(nameBuffer, name.data(), std::min((int)name.length(), HELL_NAME_BUFFER_SIZE - 1));
        nameBuffer[std::min((int)name.length(), HELL_NAME_BUFFER_SIZE - 1)] = '\0';
    }

    void MemCopyFileSignature(char* signatureBuffer, const std::string& signatureName) {
        std::memset(signatureBuffer, 0, HELL_SIGNATURE_BUFFER_SIZE);
        std::memcpy(signatureBuffer, signatureName.data(), signatureName.length());
        signatureBuffer[signatureName.length()] = '\0';
    }

    bool CompareFileSignature(char* signatureBuffer, const std::string& signatureName) {
        if (signatureName.length() >= HELL_SIGNATURE_BUFFER_SIZE) return false;

        char expectedSignature[HELL_SIGNATURE_BUFFER_SIZE];
        std::memset(expectedSignature, 0, HELL_SIGNATURE_BUFFER_SIZE);
        std::memcpy(expectedSignature, signatureName.data(), signatureName.length());
        return (std::memcmp(signatureBuffer, expectedSignature, HELL_SIGNATURE_BUFFER_SIZE) == 0);
    }

    bool ValidateSignature(const std::string& signature, const std::string& expectedSignature) {
        if (signature.size() < expectedSignature.size()) return false;
        return std::memcmp(signature.data(), expectedSignature.data(), expectedSignature.size()) == 0;
    }

    std::string ReadFileHeaderSignature(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return "";

        char fileSignature[32];
        if (!file.read(fileSignature, sizeof(fileSignature))) return "";

        return std::string(fileSignature, sizeof(fileSignature));
    }

    uint32_t ReadFileHeaderVersion(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return 0;

        // Skip fixed 32-byte signature
        file.seekg(32, std::ios::beg);
        if (!file) return 0;

        uint32_t version = 0;
        if (!file.read(reinterpret_cast<char*>(&version), sizeof(version))) return 0;
        return version;
    }
}

/*
█▄ ▄█ █▀█ █▀▄ █▀▀ █   █▀▀
█ █ █ █ █ █ █ █▀▀ █   ▀▀█
▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ ▀▀▀ */


void File::ExportSkinnedModel(const SkinnedModelData& skinnedModelData) {
    std::string outputPath = "res/skinned_models/" + skinnedModelData.name + ".skinnedmodel";
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to open file for writing: " << outputPath << "\n";
        return;
    }

    // Populate header
    SkinnedModelHeader modelHeader;
    modelHeader.version = 1;
    modelHeader.meshCount = skinnedModelData.GetMeshCount();
    modelHeader.nodeCount = skinnedModelData.GetNodeCount();
    modelHeader.boneCount = skinnedModelData.GetBoneCount();
    modelHeader.vertexCount = skinnedModelData.vertexCount;
    modelHeader.indexCount = skinnedModelData.indexCount;
    modelHeader.nameLength = skinnedModelData.name.size();
    modelHeader.timestamp = skinnedModelData.timestamp;
    std::memcpy(modelHeader.signature, "HELL_SKINNED_MODEL", 18);

    // Write header
    file.write(modelHeader.signature, 18);
    file.write((char*)&modelHeader.version, sizeof(modelHeader.version));
    file.write((char*)&modelHeader.nameLength, sizeof(modelHeader.nameLength));
    file.write((char*)&modelHeader.vertexCount, sizeof(modelHeader.vertexCount));
    file.write((char*)&modelHeader.indexCount, sizeof(modelHeader.indexCount));
    file.write((char*)&modelHeader.meshCount, sizeof(modelHeader.meshCount));
    file.write((char*)&modelHeader.nodeCount, sizeof(modelHeader.nodeCount));
    file.write((char*)&modelHeader.boneCount, sizeof(modelHeader.boneCount));
    file.write(reinterpret_cast<char*>(&modelHeader.timestamp), sizeof(modelHeader.timestamp));

    // Write model name
    file.write(skinnedModelData.name.data(), modelHeader.nameLength);

    // Write nodes
    for (const Node& node : skinnedModelData.nodes) {
        uint32_t nodeNameLength = static_cast<uint32_t>(node.name.length());
        file.write(reinterpret_cast<const char*>(&nodeNameLength), sizeof(nodeNameLength));
        file.write(node.name.c_str(), nodeNameLength);
        file.write(reinterpret_cast<const char*>(&node.parentIndex), sizeof(node.parentIndex));
        file.write(reinterpret_cast<const char*>(&node.inverseBindTransform), sizeof(node.inverseBindTransform));
    }

    // Write the bone offsets
    for (const glm::mat4& boneOffset : skinnedModelData.boneOffsets) {;
        file.write(reinterpret_cast<const char*>(&boneOffset), sizeof(boneOffset));
    }

    // Write the bone mapping
    for (const auto& pair : skinnedModelData.boneMapping) {
        const std::string& boneName = pair.first;
        uint32_t nameLength = static_cast<uint32_t>(boneName.size());
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        file.write(boneName.data(), nameLength);
        file.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
    }

    #if PRINT_SKINNED_MODEL_HEADERS_ON_WRITE
    PrintSkinnedModelHeader(modelHeader, "Wrote skinned model header: " + skinnedModelData.name);
    #endif

    std::string name;
    std::vector<WeightedVertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t localBaseVertex;

    // Write the mesh data
    for (const SkinnedMeshData& skinnedMeshData : skinnedModelData.meshes) {
        SkinnedMeshHeader skinnedMeshHeader;
        skinnedMeshHeader.nameLength = (uint32_t)skinnedMeshData.name.size();
        skinnedMeshHeader.vertexCount = (uint32_t)skinnedMeshData.vertices.size();
        skinnedMeshHeader.indexCount = (uint32_t)skinnedMeshData.indices.size();
        skinnedMeshHeader.localBaseVertex = skinnedMeshData.localBaseVertex;
        skinnedMeshHeader.aabbMin = skinnedMeshData.aabbMin;
        skinnedMeshHeader.aabbMax = skinnedMeshData.aabbMax;
        skinnedMeshHeader.requiresSkinning = skinnedMeshData.requiresSkinning;
        skinnedMeshHeader.nonDeformingBoneIndex = skinnedMeshData.nonDeformingBoneIndex;

        file.write((char*)&skinnedMeshHeader.nameLength, sizeof(skinnedMeshHeader.nameLength));
        file.write((char*)&skinnedMeshHeader.vertexCount, sizeof(skinnedMeshHeader.vertexCount));
        file.write((char*)&skinnedMeshHeader.indexCount, sizeof(skinnedMeshHeader.indexCount));
        file.write((char*)&skinnedMeshHeader.localBaseVertex, sizeof(skinnedMeshHeader.localBaseVertex));
        file.write(skinnedMeshData.name.data(), skinnedMeshHeader.nameLength);
        file.write(reinterpret_cast<const char*>(&skinnedMeshHeader.aabbMin), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&skinnedMeshHeader.aabbMax), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&skinnedMeshHeader.requiresSkinning), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&skinnedMeshHeader.nonDeformingBoneIndex), sizeof(int32_t));
        file.write(reinterpret_cast<const char*>(skinnedMeshData.vertices.data()), skinnedMeshData.vertices.size() * sizeof(WeightedVertex));
        file.write(reinterpret_cast<const char*>(skinnedMeshData.indices.data()), skinnedMeshData.indices.size() * sizeof(uint32_t));

        //file.write((char*)&skinnedMeshHeader.requiresSkinning, sizeof(skinnedMeshHeader.requiresSkinning));

        #if PRINT_SKINNED_MESH_HEADERS_ON_WRITE
            PrintSkinnedMeshHeader(skinnedMeshHeader, "Wrote skinned mesh: " + skinnedMeshData.name);
        #endif
    }
    file.close();
    std::cout << "Exported: " << outputPath << "\n";
}

SkinnedModelData File::ImportSkinnedModel(const std::string& filepath) {
    SkinnedModelData skinnedModelData;

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ile::ImportSkinnedModel() failed: Could not open: " << filepath << "\n";
        return skinnedModelData;
    }
    
    SkinnedModelHeader skinnedModelHeader;

    // Read 18 bytes for HELL_SKINNED_MODEL
    char fileSignature[18];
    file.read(fileSignature, 18);

    // Read the rest of the header
    file.read((char*)&skinnedModelHeader.version, sizeof(skinnedModelHeader.version));
    file.read((char*)&skinnedModelHeader.nameLength, sizeof(skinnedModelHeader.nameLength));
    file.read((char*)&skinnedModelHeader.vertexCount, sizeof(skinnedModelHeader.vertexCount));
    file.read((char*)&skinnedModelHeader.indexCount, sizeof(skinnedModelHeader.indexCount));
    file.read((char*)&skinnedModelHeader.meshCount, sizeof(skinnedModelHeader.meshCount));
    file.read((char*)&skinnedModelHeader.nodeCount, sizeof(skinnedModelHeader.nodeCount));
    file.read((char*)&skinnedModelHeader.boneCount, sizeof(skinnedModelHeader.boneCount));
    file.read(reinterpret_cast<char*>(&skinnedModelHeader.timestamp), sizeof(skinnedModelHeader.timestamp));

    // Read the name
    std::string modelName(skinnedModelHeader.nameLength, '\0');
    if (skinnedModelHeader.nameLength > 0) {
        file.read(&modelName[0], skinnedModelHeader.nameLength);
    }

    #if PRINT_SKINNED_MODEL_HEADERS_ON_READ
    PrintSkinnedModelHeader(skinnedModelHeader, "Read model header: '" + modelName + "'");
    #endif

    // Store header data
    skinnedModelData.name = modelName;
    skinnedModelData.vertexCount = skinnedModelHeader.vertexCount;
    skinnedModelData.indexCount = skinnedModelHeader.indexCount;
    skinnedModelData.meshes.resize(skinnedModelHeader.meshCount);
    skinnedModelData.nodes.resize(skinnedModelHeader.nodeCount);
    skinnedModelData.boneOffsets.resize(skinnedModelHeader.boneCount);
    skinnedModelData.timestamp = skinnedModelHeader.timestamp;

    // Read the nodes
    for (uint32_t i = 0; i < skinnedModelHeader.nodeCount; i++) {
        uint32_t nodeNameLength = 0;
        file.read(reinterpret_cast<char*>(&nodeNameLength), sizeof(nodeNameLength));

        // Read node name
        std::string nodeName;
        nodeName.resize(nodeNameLength);
        if (nodeNameLength > 0) {
            file.read(&nodeName[0], nodeNameLength);
        }

        // Read parent index
        int parentIndex = 0;
        file.read(reinterpret_cast<char*>(&parentIndex), sizeof(parentIndex));

        // Read the inverse bind transform
        glm::mat4 inverseBindTransform;
        file.read(reinterpret_cast<char*>(&inverseBindTransform), sizeof(inverseBindTransform));

        Node node;
        node.name = nodeName;
        node.parentIndex = parentIndex;
        node.inverseBindTransform = inverseBindTransform;
        skinnedModelData.nodes[i] = node;

        //std::cout << i << " " << nodeName << "\n";
        //std::cout << "parent Index: " << parentIndex << "\n";
        //std::cout << Util::Mat4ToString(inverseBindTransform) << "\n\n";
    }

    // Read the bone offsets
    for (uint32_t i = 0; i < skinnedModelHeader.boneCount; i++) {
        file.read(reinterpret_cast<char*>(&skinnedModelData.boneOffsets[i]), sizeof(glm::mat4));

        //std::cout << i << "\n" << Util::Mat4ToString(skinnedModelData.boneOffsets[i]) << "\n\n";
    }

    // Read the bone mapping
    for (uint32_t i = 0; i < skinnedModelHeader.boneCount; i++) {
        uint32_t nameLength = 0;
        file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

        // Read the bone name into a string
        std::string boneName;
        boneName.resize(nameLength);
        if (nameLength > 0) {
            file.read(&boneName[0], nameLength);
        }

        // Read the corresponding bone index
        unsigned int boneIndex = 0;
        file.read(reinterpret_cast<char*>(&boneIndex), sizeof(boneIndex));

        // Insert into the bone mapping
        skinnedModelData.boneMapping[boneName] = boneIndex;

        //std::cout << i << " " << boneName << ": " << boneIndex << "\n";
    }

    for (uint32_t i = 0; i < skinnedModelHeader.meshCount; i++) {
        // Read the header values for this mesh.
        uint32_t nameLength = 0;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t localBaseVertex = 0;
        bool requiresSkinning = false;
        int32_t nonDeformingBoneIndex = -1;

        file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        file.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
        file.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
        file.read(reinterpret_cast<char*>(&localBaseVertex), sizeof(localBaseVertex));

        // Read the mesh name.
        std::string meshName(nameLength, '\0');
        if (nameLength > 0) {
            file.read(&meshName[0], nameLength);
        }

        // Read the AABB values.
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
        file.read(reinterpret_cast<char*>(&aabbMin), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&aabbMax), sizeof(glm::vec3));

        // Read the "requiresSkinning" value and non-deforming bone index
        file.read(reinterpret_cast<char*>(&requiresSkinning), sizeof(bool));
        file.read(reinterpret_cast<char*>(&nonDeformingBoneIndex), sizeof(int32_t));
        
        // Now set up the mesh data.
        SkinnedMeshData& meshData = skinnedModelData.meshes[i];
        meshData.name = meshName;
        meshData.vertexCount = vertexCount;
        meshData.indexCount = indexCount;
        meshData.localBaseVertex = localBaseVertex;
        meshData.aabbMin = aabbMin;
        meshData.aabbMax = aabbMax;
        meshData.requiresSkinning = requiresSkinning;
        meshData.nonDeformingBoneIndex = nonDeformingBoneIndex;

        // Allocate space for vertices and indices.
        meshData.vertices.resize(vertexCount);
        meshData.indices.resize(indexCount);

        // Read the vertices and indices.
        file.read(reinterpret_cast<char*>(meshData.vertices.data()), vertexCount * sizeof(WeightedVertex));
        file.read(reinterpret_cast<char*>(meshData.indices.data()), indexCount * sizeof(uint32_t));


        //std::cout << i << ": " << meshData.name << " ";
        //std::cout << meshData.vertices.size() << " verts, ";
        //std::cout << meshData.indices.size() << " indices, ";
        //std::cout << Util::Vec3ToString(meshData.aabbMin) << " ";
        //std::cout << Util::Vec3ToString(meshData.aabbMax) << "\n";

        //#if PRINT_SKINNED_MESH_HEADERS_ON_READ
        //    PrintSkinnedMeshHeader(meshData, "Read skinned mesh: " + meshData.name);
        //#endif
    }

    return skinnedModelData;
}

SkinnedModelHeader File::ReadSkinnedModelHeader(const std::string& filepath) {
    SkinnedModelHeader header{};
    std::ifstream file(filepath, std::ios::binary);

    // Check for invalid file
    if (!file.is_open()) {
        std::cout << "File::ReadSkinnedModelHeader() failed: could not open " << filepath << "\n";
        return header;
    }

    // Check for valid signature
    char fileSignature[18];
    file.read(fileSignature, 18);
    if (std::strncmp(fileSignature, "HELL_SKINNED_MODEL", 18) != 0) {
        std::cout << "File::ReadSkinnedModelHeader() failed: 'HELL_SKINNED_MODEL' signature not found in header\n";
        return header;
    }

    // Read the rest of header
    file.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
    file.read(reinterpret_cast<char*>(&header.nameLength), sizeof(header.nameLength));
    file.read(reinterpret_cast<char*>(&header.vertexCount), sizeof(header.vertexCount));
    file.read(reinterpret_cast<char*>(&header.indexCount), sizeof(header.indexCount));
    file.read(reinterpret_cast<char*>(&header.meshCount), sizeof(header.meshCount));
    file.read(reinterpret_cast<char*>(&header.nodeCount), sizeof(header.nodeCount));
    file.read(reinterpret_cast<char*>(&header.boneCount), sizeof(header.boneCount));
    file.read(reinterpret_cast<char*>(&header.timestamp), sizeof(header.timestamp));
    return header;
}

/*
 ▀█▀   █ █▀█
  █  ▄▀  █ █
 ▀▀▀ ▀   ▀▀▀ */

void File::DeleteFile(const std::string& filepath) {
    try {
        if (std::filesystem::remove(filepath)) {
            // File deleted successfully
        }
        else {
            std::cout << "File::DeleteFile() failed to delete '" << filepath << "', file does not exist or could not be deleted!\n";
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "File::DeleteFile() exception: " << e.what() << "\n";
    }
}

/*
 ▀▀█▀▀ █ █▀▄▀█ █▀▀ █▀▀ ▀█▀ █▀▀█ █▀▄▀█ █▀▀█ █▀▀
   █   █ █ ▀ █ █▀▀ ▀▀█  █  █▄▄█ █ ▀ █ █▄▄█ ▀▀█
   ▀   ▀ ▀   ▀ ▀▀▀ ▀▀▀  ▀  ▀  ▀ ▀   ▀ ▀    ▀▀▀ */

uint64_t File::GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string File::TimestampToString(uint64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm timeStruct{};
    if (localtime_s(&timeStruct, &time) != 0) {
        return "Invalid Timestamp";
    }
    char buffer[26];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeStruct)) {
        return std::string(buffer);
    }
    return "Invalid Timestamp";
}

uint64_t File::GetLastModifiedTime(const std::string& filePath) {
    try {
        auto ftime = std::filesystem::last_write_time(filePath);
        auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
        return std::chrono::duration_cast<std::chrono::seconds>(systemTime.time_since_epoch()).count();
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading file timestamp: " << e.what() << "\n";
        return 0;
    }
}

void File::ExportMeshDataToOBJ(const std::string& filepath, const MeshData& mesh) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << "\n";
        return;
    }
    for (const auto& vertex : mesh.vertices) {
        file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
    }
    for (const auto& vertex : mesh.vertices) {
        file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
    }
    for (const auto& vertex : mesh.vertices) {
        file << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
    }
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        file << "f "
            << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << " "
            << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << " "
            << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "\n";
    }
    file.close();
    std::cout << "Exported OBJ: " << filepath << "\n";
}

void File::ExportSkinnedMeshDataToOBJ(const std::string& filepath, const SkinnedMeshData& mesh) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << "\n";
        return;
    }
    for (const auto& vertex : mesh.vertices) {
        file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
    }
    for (const auto& vertex : mesh.vertices) {
        file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
    }
    for (const auto& vertex : mesh.vertices) {
        file << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
    }
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        file << "f "
            << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << " "
            << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << " "
            << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "\n";
    }
    file.close();
    std::cout << "Exported OBJ: " << filepath << "\n";
}
