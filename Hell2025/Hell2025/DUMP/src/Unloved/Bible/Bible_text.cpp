#include "Unloved/Bible/Bible.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"

namespace Bible {
    const std::string& PickRandom(const std::vector<std::string>& vector) {
        if (vector.empty()) return UNDEFINED_STRING;

        int rand = Hell::Random::Int(0, vector.size() - 1);
        return vector[rand];
    }

    const std::string& MermaidShopGreeting() {
        static const std::vector<std::string> options = {
           "Lost boys welcome, but gotta pay the toll.",
           "Come spend your clams with the queen, Sugar.",
           "Drop your dabloons Honey, this aint a charity.",
           "I aint got all day, spill the loot.",
           "Careful, Sweetheart. I charge by the minute.",
           "Stop staring. Start paying.",
           "A private performance perhaps?",
           "Come closer.",
           "Let me warm you up."
        };

        return PickRandom(options);
    }

    const std::string& MermaidShopWeaponPurchaseConfirmationText() {
        static const std::vector<std::string> options = {
            "Think that'll keep you warm?",
            "You know where to find me.",
            "Could have had me instead.",
            "I'll be here when you're lonely.",
            "Don't drop it.",
            "Try not to die will you.",
            "Enjoy, darling.",
            "Stay alive, honey.",
            "Good boy.",
            "Don't disappoint me.",
            "Come back when you need more than just a weapon."
        };

        return PickRandom(options);
    }

    const std::string& MermaidShopFailedPurchaseText() {
        static const std::vector<std::string> options = {
            "Hold your seahorses."
        };

        return PickRandom(options);
    }
}
