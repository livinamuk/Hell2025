#include "MapFile.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace Unloved::MapFile {

    void CopySignature(char* signatureBuffer, const std::string& signatureName) {
        constexpr size_t signatureBufferSize = 32;
        std::memset(signatureBuffer, 0, signatureBufferSize);
        std::memcpy(signatureBuffer, signatureName.data(), std::min(signatureName.size(), signatureBufferSize - 1));
    }
}
