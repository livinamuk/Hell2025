/*

███    █▄  ███▄▄▄▄    ▄█        ▄██████▄   ▄█    █▄     ▄████████ ████████▄
███    ███ ███▀▀▀██▄ ███       ███    ███ ███    ███   ███    ███ ███   ▀███
███    ███ ███   ███ ███       ███    ███ ███    ███   ███    █▀  ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███  ▄███▄▄▄     ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███ ▀▀███▀▀▀     ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███   ███    █▄  ███    ███N
███    ███ ███   ███ ███▌    ▄ ███    ███ ███    ███   ███    ███ ███   ▄███
████████▀   ▀█   █▀  █████▄▄██  ▀██████▀   ▀██████▀    ██████████ ████████▀

*/

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Profiling/CPUProfiler.h"

#include "Unloved/Unloved.h"

#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

int main() {
    std::cout << "\x1B[2J\x1B[H";
    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";

    // Engine Init
    if (!Hell::BackEnd::Init(API::OPENGL, WindowedMode::WINDOWED, "Unloved", 2048)) {
        return -1;
    }

    // Game Init
    if (!Unloved::Init()) {
        return -1;
    }

    // Program loop
    while (Hell::BackEnd::WindowIsOpen()) {
        ProfilerCPUFrame();

        Hell::BackEnd::BeginFrame();

        Unloved::BeginFrame();
        Unloved::Update();
        Unloved::Render();
        Unloved::EndFrame();

        Hell::BackEnd::EndFrame();
    }

    // Clean Up
    Unloved::CleanUp();
    Hell::BackEnd::CleanUp();

    return 0;
}
