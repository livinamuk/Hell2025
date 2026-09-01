#include "MaterialBuilder.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include <string>
#include <string_view>

namespace Hell::MaterialBuilder {

    namespace {
        enum class TextureType {
            ALB,
            NRM,
            RMA,
            EMI,
            OPA,
            HAR,
            DSP,
            UNDEFINED
        };

        TextureType GetTextureType(const std::string& textureName) {
            if (textureName.size() < 5 || textureName[textureName.size() - 4] != '_') {
                return TextureType::UNDEFINED;
            }

            const std::string_view suffix(textureName.data() + textureName.size() - 3, 3);

            if (suffix == "ALB") return TextureType::ALB;
            if (suffix == "NRM") return TextureType::NRM;
            if (suffix == "RMA") return TextureType::RMA;
            if (suffix == "EMI") return TextureType::EMI;
            if (suffix == "OPA") return TextureType::OPA;
            if (suffix == "HAR") return TextureType::HAR;
            if (suffix == "DSP") return TextureType::DSP;

            return TextureType::UNDEFINED;
        }

        void ApplyDefaultTextures(Material& material) {
            if (material.m_basecolor == -1)     material.m_basecolor = ResourceManager::GetTextureBindlessIndexByName("CheckerBoard_ALB");
            if (material.m_normal == -1)        material.m_normal = ResourceManager::GetTextureBindlessIndexByName("DefaultNRM");
            if (material.m_rma == -1)           material.m_rma = ResourceManager::GetTextureBindlessIndexByName("DefaultRMA");
            if (material.m_emissive == -1)      material.m_emissive = ResourceManager::GetTextureBindlessIndexByName("Black");
            if (material.m_opacity == -1)       material.m_opacity = ResourceManager::GetTextureBindlessIndexByName("White");
            if (material.m_hairMaps == -1)      material.m_hairMaps = ResourceManager::GetTextureBindlessIndexByName("Black");
            if (material.m_displacement == -1)  material.m_displacement = ResourceManager::GetTextureBindlessIndexByName("Black"); // Find out what the best default is
        }
    }

    void RegisterTexture(const Texture& texture) {
        const std::string& textureName = texture.GetFileName();
        const TextureType textureType = GetTextureType(textureName);

        if (textureType != TextureType::UNDEFINED) {
            const std::string materialName = textureName.substr(0, textureName.size() - 4);
            int32_t materialIndex = ResourceManager::GetMaterialIndexByName(materialName);

            if (materialIndex == -1) {
                ResourceManager::CreateMaterial(materialName);
                materialIndex = ResourceManager::GetMaterialIndexByName(materialName);
            }

            Material* material = ResourceManager::GetMaterialByIndex(materialIndex);
            if (material) {
                const int32_t bindlessIndex = texture.GetBindlessIndex();

                switch (textureType) {
                    case TextureType::ALB: material->m_basecolor = bindlessIndex; break;
                    case TextureType::NRM: material->m_normal = bindlessIndex; break;
                    case TextureType::RMA: material->m_rma = bindlessIndex; break;
                    case TextureType::EMI: material->m_emissive = bindlessIndex; break;
                    case TextureType::OPA: material->m_opacity = bindlessIndex; break;
                    case TextureType::HAR: material->m_hairMaps = bindlessIndex; break;
                    case TextureType::DSP: material->m_displacement = bindlessIndex; break;
                    case TextureType::UNDEFINED: break;
                }
            }
        }

        for (Material& material : ResourceManager::GetMaterials()) {
            ApplyDefaultTextures(material);
        }
    }
}
