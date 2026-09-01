#include "FeatureTest.h"

#include "Unloved/Common/Constants.h"

#include "Hell/UI/UIBackEnd.h"

namespace FeatureTest {
    
    void InventoryTextTest();

    void Update(float deltaTime) {
        InventoryTextTest();
    }

    void InventoryTextTest() {
        UIBackEnd::BlitTexture("inv2", { 745, 100 }, Alignment::TOP_LEFT, WHITE, { -1, -1 }, TextureFilter::LINEAR);

        std::string name = "GLOCK 22";
        std::string description = R"(Australian police issue. Matte and boxy, a cold
little companion. It does the paperwork duty
without drama. Dependable at short range,
underwhelming at a distance. A proper piece
of shit.)";

        name = "9 X 19MM";
        description = R"(Born for Lugers, adopted by everyone. NATO's
common tongue, cheap to feed and easy to
stack.
)";

        name = "7.62 X 25MM";
        description = R"(Long, bottleneck case; hot, flat, and loud.
Punches deep, too deep, through coats, doors,
and sometimes sense.
)";

        name = "AKS74U";
        description = R"(Krinkov to some, a headache to anyone nearby.
Built for tank hatches, stairwells, and mowing
yer enemy like meat.
)";

        name = "SHOTGUN SHELLS";
        description = R"(12 gauge, plastic hulls with brass rims. They
pattern wide and peel flesh. Heavy on the
shoulder, honest in the work.
)";

        name = "RED DOT SIGHT";
        description = R"(A little window with a floating promise. Turns
aim into point, and point into a bloody mess.
)";

        name = "SUPPRESSOR";
        /*description = R"(A metal hush that threads onto bad intentions.
You'll hear the brass breathe before it even hits
the floor.
)";*/
        description = R"(You'll hear the brass breathe before it even
hits the floor.
)";

        UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + name, "BebasNeue", 800, 438, Alignment::TOP_LEFT, 1.0f, TextureFilter::LINEAR);
        UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + description, "RobotoCondensed", 800, 486, Alignment::TOP_LEFT, 1.0f, TextureFilter::LINEAR);

        UIBackEnd::BlitTexture("inventory_green_button", glm::ivec2(850, 630), Alignment::CENTERED, WHITE, glm::ivec2(-1, -1), TextureFilter::LINEAR);
        UIBackEnd::BlitText("[COL=0.839,0.784,0.635]M", "RobotoCondensed", 850, 631, Alignment::CENTERED, 0.75f, TextureFilter::LINEAR);
        UIBackEnd::BlitText("[COL=0.839,0.784,0.635]Move", "RobotoCondensed", 870, 631, Alignment::CENTERED_VERTICAL, 1.0f, TextureFilter::LINEAR);

        UIBackEnd::BlitTexture("inventory_green_button", glm::ivec2(850, 660), Alignment::CENTERED, WHITE, glm::ivec2(-1, -1), TextureFilter::LINEAR);
        UIBackEnd::BlitText("[COL=0.839,0.784,0.635]C", "RobotoCondensed", 850, 661, Alignment::CENTERED, 0.75f, TextureFilter::LINEAR);
        UIBackEnd::BlitText("[COL=0.839,0.784,0.635]Combine", "RobotoCondensed", 870, 661, Alignment::CENTERED_VERTICAL, 1.0f, TextureFilter::LINEAR);
    }
}
