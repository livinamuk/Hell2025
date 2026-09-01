#include "AssetLoader.h"

#include "Hell/Audio/Synth.h"
#include "Hell/File.h"
#include "Hell/Logging.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/SoundFont.h"

#include <utility>

namespace Hell::AssetLoader {

    void LoadSoundFonts() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/audio/piano", { "sf2" })) {
            SoundFont soundFont = Hell::Synth::LoadSoundFont(fileInfo.path);

            if (soundFont.GetFluidSynthId() < 0) {
                Logging::Error() << "AssetLoader::LoadSoundFonts() failed to load '" << fileInfo.path << "'\n";
                continue;
            }

            ResourceManager::CreateSoundFont(std::move(soundFont));
        }
    }
}
