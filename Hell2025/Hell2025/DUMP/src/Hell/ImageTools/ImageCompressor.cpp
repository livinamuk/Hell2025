#include "ImageTools.h"

#include "cmp_compressonatorlib/compressonator.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>

namespace Hell::ImageTools {

    namespace {

    std::once_flag g_cmpInitializationFlag;
    std::mutex g_cmpMutex;

    void EnsureCMPFrameworkInitialized() {
        std::call_once(g_cmpInitializationFlag, [] {
            CMP_InitFramework();
        });
    }

    bool CompressionCallback(CMP_FLOAT progress, CMP_DWORD_PTR, CMP_DWORD_PTR) {
        std::printf("\rCompression progress = %3.0f", progress);
        return false;
    }

    const char* CMPErrorToString(CMP_ERROR error) {
        switch (error) {
            case CMP_OK:                               return "Ok.";
            case CMP_ABORTED:                          return "The conversion was aborted.";
            case CMP_ERR_INVALID_SOURCE_TEXTURE:       return "The source texture is invalid.";
            case CMP_ERR_INVALID_DEST_TEXTURE:         return "The destination texture is invalid.";
            case CMP_ERR_UNSUPPORTED_SOURCE_FORMAT:    return "The source format is not supported.";
            case CMP_ERR_UNSUPPORTED_DEST_FORMAT:      return "The destination format is not supported.";
            case CMP_ERR_UNSUPPORTED_GPU_ASTC_DECODE:  return "The GPU does not support ASTC decoding.";
            case CMP_ERR_UNSUPPORTED_GPU_BASIS_DECODE: return "The GPU does not support Basis decoding.";
            case CMP_ERR_SIZE_MISMATCH:                return "The source and destination sizes do not match.";
            case CMP_ERR_UNABLE_TO_INIT_CODEC:         return "Compressonator could not initialize the codec.";
            case CMP_ERR_UNABLE_TO_INIT_DECOMPRESSLIB: return "Compressonator could not initialize its decompression library.";
            case CMP_ERR_UNABLE_TO_INIT_COMPUTELIB:    return "Compressonator could not initialize its compute library.";
            case CMP_ERR_CMP_DESTINATION:              return "Compressonator failed while encoding the destination.";
            case CMP_ERR_MEM_ALLOC_FOR_MIPSET:         return "Compressonator could not allocate the mip set.";
            case CMP_ERR_UNKNOWN_DESTINATION_FORMAT:   return "The destination format is unknown.";
            case CMP_ERR_FAILED_HOST_SETUP:            return "Compressonator host setup failed.";
            case CMP_ERR_PLUGIN_FILE_NOT_FOUND:        return "A required Compressonator plugin was not found.";
            case CMP_ERR_UNABLE_TO_LOAD_FILE:          return "Compressonator could not load the file.";
            case CMP_ERR_UNABLE_TO_CREATE_ENCODER:     return "Compressonator could not create the encoder.";
            case CMP_ERR_UNABLE_TO_LOAD_ENCODER:       return "Compressonator could not load the encoder.";
            case CMP_ERR_NOSHADER_CODE_DEFINED:        return "No shader code is available for the selected framework.";
            case CMP_ERR_GPU_DOESNOT_SUPPORT_COMPUTE:  return "The GPU does not support the requested compute path.";
            case CMP_ERR_NOPERFSTATS:                  return "No performance statistics are available.";
            case CMP_ERR_GPU_DOESNOT_SUPPORT_CMP_EXT:  return "The GPU does not support the requested compression extension.";
            case CMP_ERR_GAMMA_OUTOFRANGE:             return "The gamma value is out of range.";
            case CMP_ERR_PLUGIN_SHAREDIO_NOT_SET:      return "Compressonator plugin shared IO is not configured.";
            case CMP_ERR_UNABLE_TO_INIT_D3DX:          return "Compressonator could not initialize D3DX.";
            case CMP_FRAMEWORK_NOT_INITIALIZED:        return "The Compressonator framework is not initialized.";
            case CMP_ERR_GENERIC:                      return "An unknown Compressonator error occurred.";
            default:                                   return "Unknown Compressonator error.";
        }
    }

    }

    void CreateAndExportDDS(const std::string& inputFilepath, const std::string& outputFilepath, bool generateMipMaps) {
        std::lock_guard<std::mutex> lock(g_cmpMutex);
        EnsureCMPFrameworkInitialized();

        CMP_MipSet mipSetIn = {};
        CMP_MipSet mipSetOut = {};
        bool inputLoaded = false;
        bool outputCreated = false;

        auto cleanup = [&] {
            if (inputLoaded) {
                CMP_FreeMipSet(&mipSetIn);
            }
            if (outputCreated) {
                CMP_FreeMipSet(&mipSetOut);
            }
        };

        CMP_ERROR status = CMP_LoadTexture(inputFilepath.c_str(), &mipSetIn);
        if (status != CMP_OK) {
            std::cout << "Failed to load texture " << inputFilepath << ": " << CMPErrorToString(status) << "\n";
            return;
        }
        inputLoaded = true;

        if (generateMipMaps) {
            const CMP_INT mipLevelCount =
                static_cast<CMP_INT>(std::log2(std::max(mipSetIn.m_nWidth, mipSetIn.m_nHeight))) + 1;
            const CMP_INT minimumMipSize =
                CMP_CalcMinMipSize(mipSetIn.m_nHeight, mipSetIn.m_nWidth, mipLevelCount);
            CMP_GenerateMIPLevels(&mipSetIn, minimumMipSize);
        }

        KernelOptions options = {};
        options.encodeWith = CMP_HPC;
        options.format = CMP_FORMAT_BC7;
        options.fquality = 0.88f;
        options.threads = 0;

        std::memset(&mipSetOut, 0, sizeof(mipSetOut));
        outputCreated = true;
        status = CMP_ProcessTexture(&mipSetIn, &mipSetOut, options, CompressionCallback);
        std::cout << "\n";
        if (status != CMP_OK) {
            std::cout << "Failed to compress texture " << inputFilepath << ": " << CMPErrorToString(status) << "\n";
            cleanup();
            return;
        }

        status = CMP_SaveTexture(outputFilepath.c_str(), &mipSetOut);
        if (status != CMP_OK) {
            std::cout << "Failed to save texture " << outputFilepath << ": " << CMPErrorToString(status) << "\n";
        }

        cleanup();
    }
}
