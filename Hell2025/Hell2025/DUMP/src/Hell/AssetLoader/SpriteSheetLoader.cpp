#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace Hell::AssetLoader {

    void CreateSpriteSheets() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/spritesheets", { "png", "jpg", "tga" })) {
            if (ResourceManager::GetSpriteSheetTexturePtr(fileInfo.name)) {
                continue;
            }

            Texture* texture = ResourceManager::GetTextureByName(fileInfo.name);

            if (!texture) {
                AddLoadLogItem("Failed to create spritesheet " + fileInfo.path);
                Logging::Error() << "AssetLoader::CreateSpriteSheets() failed: texture '" << fileInfo.name << "' does not exist\n";
                continue;
            }

            if (texture->GetUploadState() != UploadState::UPLOADED) {
                AddLoadLogItem("Failed to create spritesheet " + fileInfo.path);
                Logging::Error() << "AssetLoader::CreateSpriteSheets() failed: texture '" << fileInfo.name << "' is not uploaded\n";
                continue;
            }

            SpriteSheetTexture& spriteSheetTexture = ResourceManager::CreateSpriteSheetTexture(fileInfo.name);
            spriteSheetTexture.SetFileInfo(fileInfo);
            spriteSheetTexture.Init(*texture);
            AddLoadLogItem("Created spritesheet " + fileInfo.path);
        }
    }
}
