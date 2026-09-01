#include "VAT.h"

namespace Hell {

    Vat::Vat(const std::string& name) {
        m_name = name;
    }

    size_t Vat::GetCPUAllocatedByteCount() const {
        size_t byteCount = m_name.capacity() +
            m_fileInfo.path.capacity() +
            m_fileInfo.name.capacity() +
            m_fileInfo.ext.capacity() +
            m_fileInfo.dir.capacity() +
            m_metadata.positionTexture.capacity() +
            m_metadata.rotationTexture.capacity() +
            m_metadata.lookupTexture.capacity() +
            m_metadata.model.capacity();

        return byteCount;
    }

}
