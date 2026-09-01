#include "Physics.h"
#include "Hell/Physics/Types/HeightField.h"

namespace Hell::Physics {

void ActivateAllHeightFields() {
        for (HeightField& heightField : GetHeightFields()) {
            heightField.ActivatePhsyics();
        }
    }
}
