#include "D6Joint.h"

void D6Joint::SetPxD6Joint(PxD6Joint* pxD6Joint) {
    m_pxD6Joint = pxD6Joint;
}

void D6Joint::MarkForRemoval() {
    m_markedForRemoval = true;
}
