#pragma once

#include "Unloved/Maps/MapFileFormat.h"

#include <string>

namespace Unloved::MapFile {

    void CopySignature(char* signatureBuffer, const std::string& signatureName);
}
