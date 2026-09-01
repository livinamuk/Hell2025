#include "Unloved/Bible/Bible.h"

namespace Unloved::Bible {
    using namespace Unloved;


	void ConfigureP90MagazineMeshNodes(uint64_t id, MeshNodes* meshNodes) {

		std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

		for (int i = 1; i <= 51; i++) {
			MeshNodeCreateInfo& bullet = meshNodeCreateInfoSet.emplace_back();
			bullet.materialName = "P90_Mag";

			if (i < 10) {
				bullet.meshName = "Bullet_0" + std::to_string(i);
			}
			else {
				bullet.meshName = "Bullet_" +std::to_string(i);
			}
		}

		MeshNodeCreateInfo& spring = meshNodeCreateInfoSet.emplace_back();
		spring.materialName = "P90_Mag";
		spring.meshName = "P90_MagazineSpring";

		MeshNodeCreateInfo& springFollower = meshNodeCreateInfoSet.emplace_back();
		springFollower.materialName = "P90_Mag";
		springFollower.meshName = "P90_MagazineSpringFollower";

		MeshNodeCreateInfo& magazine = meshNodeCreateInfoSet.emplace_back();
		magazine.materialName = "P90_Mag";
		magazine.meshName = "P90_Magazine";

		meshNodes->Init(id, "P90_Magazine", meshNodeCreateInfoSet);

		meshNodes->DisableMarkingStaticSceneBvhAsDirty();
		meshNodes->DisableCSMShadows();
		meshNodes->DisablePointLightShadows();
	}
}
