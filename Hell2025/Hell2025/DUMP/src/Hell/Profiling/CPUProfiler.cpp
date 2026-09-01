#include "CPUProfiler.h"

#include "Hell/Common/String.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace Hell::CPUProfiler {
    namespace {
        using Clock = std::chrono::steady_clock;

        constexpr size_t kAverageFrameCount = 60;
        constexpr uint32_t kMaxTimingColumnDepth = 6;
        constexpr uint32_t kInvalidNodeIndex = UINT32_MAX;

        struct RollingAverage {
            std::deque<double> samples;
            double sum = 0.0;

            void Push(double value) {
                samples.push_back(value);
                sum += value;

                while (samples.size() > kAverageFrameCount) {
                    sum -= samples.front();
                    samples.pop_front();
                }
            }

            double GetValue() const {
                return samples.empty() ? 0.0 : sum / static_cast<double>(samples.size());
            }
        };

        struct ZoneNode {
            std::string name;
            std::string file;
            uint32_t line = 0;
            uint32_t parentIndex = kInvalidNodeIndex;
            uint32_t depth = 0;
            std::vector<uint32_t> children;
            RollingAverage rollingAverage;
            double currentMilliseconds = 0.0;
            uint32_t currentCallCount = 0;
            uint64_t lastTouchedFrame = 0;
        };

        struct ActiveZone {
            uint32_t nodeIndex = kInvalidNodeIndex;
            Clock::time_point startTime{};
        };

        struct ProfilerState {
            std::vector<ZoneNode> nodes;
            std::vector<ActiveZone> activeZones;
            Report report;
            uint64_t frameId = 0;
            bool frameActive = false;

            ProfilerState() {
                ZoneNode& root = nodes.emplace_back();
                root.name = "Frame";
                root.depth = 0;
            }
        };

        ProfilerState& GetState() {
            static ProfilerState state;
            return state;
        }

        std::string StripFunctionSignature(std::string_view signature) {
            const size_t openParenthesis = signature.find('(');
            if (openParenthesis != std::string_view::npos) {
                signature = signature.substr(0, openParenthesis);
            }

            while (!signature.empty() && signature.back() == ' ') {
                signature.remove_suffix(1);
            }

            const size_t finalSpace = signature.rfind(' ');
            if (finalSpace != std::string_view::npos) {
                signature = signature.substr(finalSpace + 1);
            }

            return std::string(signature);
        }

        std::string FormatMilliseconds(double milliseconds) {
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(2) << milliseconds << " ms";
            return stream.str();
        }

        bool IsVisible(const ProfilerState& state, const ZoneNode& node) {
            if (node.parentIndex == kInvalidNodeIndex) return true;
            if (node.lastTouchedFrame == 0) return false;
            return state.frameId - node.lastTouchedFrame < kAverageFrameCount;
        }

        std::vector<uint32_t> GetVisibleChildren(const ProfilerState& state, const ZoneNode& node) {
            std::vector<uint32_t> children;
            children.reserve(node.children.size());

            for (uint32_t childIndex : node.children) {
                if (childIndex < state.nodes.size() && IsVisible(state, state.nodes[childIndex])) {
                    children.push_back(childIndex);
                }
            }
            return children;
        }

        void AppendReportNode(ProfilerState& state, uint32_t nodeIndex, const std::string& prefix, bool isLastChild) {
            const ZoneNode& node = state.nodes[nodeIndex];

            if (node.parentIndex == kInvalidNodeIndex) {
                state.report.zoneNames += node.name;
            }
            else {
                state.report.zoneNames += prefix;
                state.report.zoneNames += isLastChild ? "  " : "  ";
                state.report.zoneNames += node.name;
            }
            if (node.currentCallCount > 1) {
                state.report.zoneNames += " (";
                state.report.zoneNames += std::to_string(node.currentCallCount);
                state.report.zoneNames += " calls)";
            }
            state.report.zoneNames += "\n";

            const uint32_t timingDepth = std::min(node.depth, kMaxTimingColumnDepth);
            for (uint32_t depth = 0; depth <= kMaxTimingColumnDepth; ++depth) {
                if (depth == timingDepth) {
                    state.report.timingColumns[depth] += FormatMilliseconds(node.rollingAverage.GetValue());
                }
                state.report.timingColumns[depth] += "\n";
            }

            const std::vector<uint32_t> visibleChildren = GetVisibleChildren(state, node);
            for (size_t child = 0; child < visibleChildren.size(); ++child) {
                const bool childIsLast = child + 1 == visibleChildren.size();
                std::string childPrefix = prefix;
                if (node.parentIndex != kInvalidNodeIndex) {
                    childPrefix += isLastChild ? "  " : "  ";
                }
                AppendReportNode(state, visibleChildren[child], childPrefix, childIsLast);
            }
        }

        void BuildReport(ProfilerState& state) {
            state.report.zoneNames.clear();
            state.report.timingColumns.assign(kMaxTimingColumnDepth + 1, {});

            AppendReportNode(state, 0, {}, true);

            uint32_t deepestUsedColumn = 0;
            for (const ZoneNode& node : state.nodes) {
                if (IsVisible(state, node)) {
                    deepestUsedColumn = std::max(deepestUsedColumn, std::min(node.depth, kMaxTimingColumnDepth));
                }
            }
            state.report.timingColumns.resize(deepestUsedColumn + 1);
        }

        bool BeginFrame() {
            ProfilerState& state = GetState();
            if (state.frameActive) return false;

            ++state.frameId;
            state.frameActive = true;
            state.activeZones.clear();

            for (ZoneNode& node : state.nodes) {
                node.currentMilliseconds = 0.0;
                node.currentCallCount = 0;
            }

            ZoneNode& root = state.nodes[0];
            root.lastTouchedFrame = state.frameId;
            root.currentCallCount = 1;
            state.activeZones.push_back({ 0, Clock::now() });
            return true;
        }

        void EndFrame() {
            ProfilerState& state = GetState();
            if (!state.frameActive || state.activeZones.empty()) return;

            const Clock::time_point endTime = Clock::now();
            const ActiveZone& rootZone = state.activeZones.front();
            state.nodes[0].currentMilliseconds =
                std::chrono::duration<double, std::milli>(endTime - rootZone.startTime).count();

            for (ZoneNode& node : state.nodes) {
                if (node.lastTouchedFrame == state.frameId) {
                    node.rollingAverage.Push(node.currentMilliseconds);
                }
            }

            state.activeZones.clear();
            state.frameActive = false;
            BuildReport(state);
        }

        uint32_t FindOrCreateNode(
            ProfilerState& state,
            uint32_t parentIndex,
            std::string_view name,
            const char* file,
            uint32_t line,
            bool functionSignature) {

            ZoneNode& parent = state.nodes[parentIndex];
            for (uint32_t childIndex : parent.children) {
                const ZoneNode& child = state.nodes[childIndex];
                if (child.line == line && child.file == file) {
                    return childIndex;
                }
            }

            ZoneNode node;
            node.name = functionSignature ? StripFunctionSignature(name) : std::string(name);
            node.file = file;
            node.line = line;
            node.parentIndex = parentIndex;
            node.depth = parent.depth + 1;

            const uint32_t nodeIndex = static_cast<uint32_t>(state.nodes.size());
            state.nodes.push_back(std::move(node));
            state.nodes[parentIndex].children.push_back(nodeIndex);
            return nodeIndex;
        }

        uint32_t BeginZone(std::string_view name, const char* file, uint32_t line, bool functionSignature) {
            ProfilerState& state = GetState();
            if (!state.frameActive || state.activeZones.empty() || name.empty()) {
                return kInvalidNodeIndex;
            }

            const uint32_t parentIndex = state.activeZones.back().nodeIndex;
            const uint32_t nodeIndex = FindOrCreateNode(state, parentIndex, name, file, line, functionSignature);

            state.nodes[nodeIndex].lastTouchedFrame = state.frameId;
            ++state.nodes[nodeIndex].currentCallCount;
            state.activeZones.push_back({ nodeIndex, Clock::now() });
            return nodeIndex;
        }

        void EndZone(uint32_t nodeIndex) {
            ProfilerState& state = GetState();
            if (!state.frameActive || state.activeZones.size() <= 1) return;

            const ActiveZone activeZone = state.activeZones.back();
            if (activeZone.nodeIndex != nodeIndex) return;

            const Clock::time_point endTime = Clock::now();
            state.nodes[nodeIndex].currentMilliseconds +=
                std::chrono::duration<double, std::milli>(endTime - activeZone.startTime).count();
            state.activeZones.pop_back();
        }
    }

    const Report& GetReport() {
        return GetState().report;
    }

    float GetZoneTime(const std::string& zoneName) {
        for (ActiveZone& zone : GetState().activeZones) {
            if (GetState().nodes[zone.nodeIndex].name == zoneName) {
                return GetState().nodes[zone.nodeIndex].rollingAverage.GetValue();
            }
        }
        return 0.0f;
    }

    FrameScope::FrameScope() {
        m_active = BeginFrame();
    }

    FrameScope::~FrameScope() {
        if (m_active) {
            EndFrame();
        }
    }

    ZoneScope::ZoneScope(std::string_view name, const char* file, uint32_t line, bool functionSignature) {
        m_nodeIndex = BeginZone(name, file, line, functionSignature);
        m_active = m_nodeIndex != kInvalidNodeIndex;
    }

    ZoneScope::~ZoneScope() {
        if (m_active) {
            EndZone(m_nodeIndex);
        }
    }
}
