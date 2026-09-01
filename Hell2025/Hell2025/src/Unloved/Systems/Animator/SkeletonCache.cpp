#include "SkeletonCache.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace Unloved::SkeletonCache {

    namespace {
        std::vector<Skeleton> g_skeletons;
        Skeleton g_emptySkeleton;
    }

    const Skeleton& GetOrCreateSkeleton(const std::vector<std::string>& skinnedModelNames) {
        if (skinnedModelNames.empty()) {
            Logging::Error() << "SkeletonCache::GetOrCreateSkeleton() requires at least one skinned model\n";
            return g_emptySkeleton;
        }

        // Resolve the skinned model names to ids
        std::vector<uint32_t> skinnedModelIds;
        skinnedModelIds.reserve(skinnedModelNames.size());

        for (const std::string& skinnedModelName : skinnedModelNames) {
            const uint32_t skinnedModelId = Hell::ResourceManager::GetSkinnedModelIdByName(skinnedModelName);

            if (skinnedModelId == 0) {
                return g_emptySkeleton;
            }

            skinnedModelIds.push_back(skinnedModelId);
        }

        // Find a skeleton built from the same ordered skinned model ids
        for (const Skeleton& skeleton : g_skeletons) {
            const std::vector<uint32_t>& cachedSkinnedModelIds = skeleton.GetSkinnedModelIds();

            if (cachedSkinnedModelIds.size() != skinnedModelIds.size()) {
                continue;
            }

            bool matches = true;

            for (uint32_t i = 0; i < skinnedModelIds.size(); i++) {
                if (cachedSkinnedModelIds[i] != skinnedModelIds[i]) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                return skeleton;
            }
        }

        // Build and cache a new skeleton
        Skeleton skeleton;
        skeleton.Build(skinnedModelIds);

        if (skeleton.GetSkinnedModelIds().empty()) {
            return g_emptySkeleton;
        }

        g_skeletons.push_back(skeleton);

        return g_skeletons.back();
    }
}
