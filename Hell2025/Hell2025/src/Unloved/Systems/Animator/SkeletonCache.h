#pragma once

#include "Unloved/Systems/Animator/Skeleton.h"

#include <string>
#include <vector>

namespace Unloved::SkeletonCache {

    const Skeleton& GetOrCreateSkeleton(const std::vector<std::string>& skinnedModelNames);
}
