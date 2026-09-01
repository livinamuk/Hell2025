#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/MidiFile.h"

#include <utility>

namespace Hell::AssetLoader {

    void LoadMidiFiles() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/audio/midi", { "mid" })) {
            MidiFile midiFile;

            if (!midiFile.LoadFromFile(fileInfo.path)) {
                Logging::Error() << "AssetLoader::LoadMidiFiles() failed to load '" << fileInfo.path << "'\n";
                continue;
            }

            ResourceManager::CreateMidiFile(std::move(midiFile));
        }
    }
}
