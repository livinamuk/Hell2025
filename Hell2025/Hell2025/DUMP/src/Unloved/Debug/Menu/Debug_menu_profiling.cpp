#include "Debug_menu.h"

#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/MemoryTracker/MemoryTracker.h"
#include "Hell/Profiling/CPUProfiler.h"

#include "Unloved/Render/Renderer.h"

#include <cstdint>
#include <string>

namespace Debug {

    void DisplayMainCPUTimingInfo() {
        constexpr char fontName[] = "StandardFont";
        constexpr float scale = 2.0f;
        constexpr int32_t margin = 35;
        constexpr int32_t timingDepthOffset = 38;
        constexpr TextureFilter textureFilter = TextureFilter::NEAREST;

        const Hell::CPUProfiler::Report& report = Hell::CPUProfiler::GetReport();
        if (report.zoneNames.empty() || report.timingColumns.empty()) return;

        const std::string headingColor = "[COL=0.56,0.93,0.56,1.0]";
        const std::string rowColor = "[COL=1.0,0.65,0.0,1.0]";
        std::string names = headingColor + "CPU TIMINGS\n" + rowColor + report.zoneNames;
        const int32_t timingStartX = TextBlitter::GetTextSize(names, fontName, scale).x + margin;

        UIBackEnd::BlitText(names, fontName, 0, 0, Alignment::TOP_LEFT, scale, textureFilter);

        for (size_t depth = 0; depth < report.timingColumns.size(); ++depth) {
            std::string timings = depth == 0 ? headingColor + "TIME\n" + rowColor : "\n" + rowColor;
            timings += report.timingColumns[depth];

            const int32_t x = timingStartX + static_cast<int32_t>(depth) * timingDepthOffset;
            UIBackEnd::BlitText(timings, fontName, x, 0, Alignment::TOP_LEFT, scale, textureFilter);
        }
    }

    void DisplayProfilingInfo() {
        constexpr float scale = 1.0f;
        constexpr int32_t margin = 35;
        constexpr TextureFilter textureFilter = TextureFilter::NEAREST;

        const std::string headingColor = "[COL=0.56,0.93,0.56,1.0]";
        const std::string zoneNames = Unloved::Renderer::GetZoneNames();
        if (zoneNames.empty()) return;

        std::string names = "\n" + zoneNames;
        std::string timingsGPU = headingColor + "GPU\n" + Unloved::Renderer::GetZoneGPUTimings();
        std::string timingsCPU = headingColor + "CPU\n" + Unloved::Renderer::GetZoneCPUTimings();
        const glm::ivec2 timingsGPUSize = TextBlitter::GetTextSize(timingsGPU, "StandardFont", scale);
        const glm::ivec2 timingsCPUSize = TextBlitter::GetTextSize(timingsCPU, "StandardFont", scale);

        timingsGPU += "\n" + headingColor + "Total GPU : " + Unloved::Renderer::GetTotalGPUTime();
        UIBackEnd::BlitText(timingsGPU, "StandardFont", 0, 0, Alignment::TOP_LEFT, scale, textureFilter);
        UIBackEnd::BlitText(timingsCPU, "StandardFont", timingsGPUSize.x + margin, 0, Alignment::TOP_LEFT, scale, textureFilter);
        UIBackEnd::BlitText(names, "StandardFont", timingsGPUSize.x + margin + timingsCPUSize.x + margin, 0, Alignment::TOP_LEFT, scale, textureFilter);
    }

    void DisplayMemoryTrackerInfo() {
        using namespace Hell::MemoryTracker;

        constexpr char fontName[] = "StandardFont";
        constexpr float scale = 2.0f;
        constexpr uint32_t spacing = 50;

        const std::string headingColor = "[COL=0.56,0.93,0.56,1.0]";
        const std::string rowColor = "[COL=1.0,0.65,0.0,1.0]";

        std::string names = "\n";
        std::string cpuBytes = headingColor + "CPU\n";
        std::string gpuBytes = headingColor + "GPU\n";
        const MemoryReport memoryReport = GetMemoryReport();

        for (const MemoryReportCategory& category : memoryReport.categories) {
            names += rowColor + category.name + "\n";
            gpuBytes += rowColor + FormatMemorySize(category.GetTotalGPUBytes()) + "\n";
            cpuBytes += rowColor + FormatMemorySize(category.GetTotalCPUBytes()) + "\n";
        }

        names += "\n" + headingColor + "Total\n";
        gpuBytes += "\n" + headingColor + FormatMemorySize(memoryReport.GetTotalGPUBytes()) + "\n";
        cpuBytes += "\n" + headingColor + FormatMemorySize(memoryReport.GetTotalCPUBytes()) + "\n";

        const int32_t namesWidth = TextBlitter::GetTextSize(names, fontName, scale).x;
        const int32_t gpuWidth = TextBlitter::GetTextSize(gpuBytes, fontName, scale).x;
        const int32_t gpuX = namesWidth + spacing;
        const int32_t cpuX = gpuX + gpuWidth + spacing;

        UIBackEnd::BlitText(names, fontName, 0, 0, Alignment::TOP_LEFT, scale);
        UIBackEnd::BlitText(gpuBytes, fontName, gpuX, 0, Alignment::TOP_LEFT, scale);
        UIBackEnd::BlitText(cpuBytes, fontName, cpuX, 0, Alignment::TOP_LEFT, scale);
    }
}

namespace Debug::Menu::Profiling {

    enum struct Setting : uint32_t {
        GPU_CPU_TIMINGS,
        MEMORY_TRACKER,
        MAIN_CPU_TIMINGS,
    };

    PageId g_homePage = ROOT_PAGE_ID;
    PageId g_gpuCpuTimingsPage = ROOT_PAGE_ID;
    PageId g_memoryTrackerPage = ROOT_PAGE_ID;
    PageId g_mainCpuTimingsPage = ROOT_PAGE_ID;

    void BuildMainMenu();

    void RegisterMenu() {
        g_homePage = RegisterRootPage("Profiling", "PROFILING", BuildMainMenu, nullptr);
        g_gpuCpuTimingsPage = RegisterDisplayPage(g_homePage, Debug::DisplayProfilingInfo);
        g_memoryTrackerPage = RegisterDisplayPage(g_homePage, Debug::DisplayMemoryTrackerInfo);
        g_mainCpuTimingsPage = RegisterDisplayPage(g_homePage, Debug::DisplayMainCPUTimingInfo);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddSubMenu(static_cast<uint32_t>(Setting::MAIN_CPU_TIMINGS), "Main CPU timings", g_mainCpuTimingsPage);
        AddSubMenu(static_cast<uint32_t>(Setting::GPU_CPU_TIMINGS), "GPU / CPU timings", g_gpuCpuTimingsPage);
        AddSubMenu(static_cast<uint32_t>(Setting::MEMORY_TRACKER), "Memory tracker", g_memoryTrackerPage);
    }
}
