#pragma once

#include "Hell/Common/Constants.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved::EditorSession {

    struct EditorObjectNameEntry {
        uint64_t objectId = 0;
        std::string name;
    };

    struct EditorObjectNameGroup {
        std::string label;
        std::vector<EditorObjectNameEntry> entries;
    };

    template<typename Container>
    bool EditorNameAvailable(Container& objects, const std::string& editorName) {
        for (auto& object : objects) {
            if (object.GetCreateInfo().editorName == editorName) {
                return false;
            }
        }
        return true;
    }

    template<typename CreateInfo, typename Container>
    void AssignEditorName(CreateInfo& createInfo, Container& objects) {
        // Desired name is what's contained in the createInfo 
        std::string desiredName = createInfo.editorName;

        // If name is undefined then fall back to the default
        if (createInfo.editorName.empty() || createInfo.editorName == UNDEFINED_STRING || createInfo.editorName == "Undefined") {
            desiredName = createInfo.defaultEditorName;
        }

        // If name is available then use it
        if (EditorNameAvailable(objects, desiredName)) {
            createInfo.editorName = desiredName;
            return;
        }

        // Otherwise find the next unused numbered suffixed version
        int suffix = 2;
        while (true) {
            const std::string possibleName = desiredName + " " + std::to_string(suffix);
            if (EditorNameAvailable(objects, possibleName)) {
                createInfo.editorName = possibleName;
                return;
            }
            suffix++;
        }
    }

    std::vector<EditorObjectNameGroup>& GetEditorObjectNameGroups();
}
