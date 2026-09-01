#include "Logging.h"
#include <iostream>
#include <atomic>
#include <vector>
#if defined(_WIN32)
    #include <windows.h>
    #ifdef ERROR
    #undef ERROR
    #endif
#endif

namespace Logging {
    std::atomic<uint32_t> g_levelMask { 0 };
    bool g_vtInitialized = false;

    const char* colorReset = "\x1b[0m";
    const char* cyan = "\x1b[36m";
    const char* blue = "\x1b[34m";
    const char* bblue = "\x1b[38;5;39m";
    const char* yellow = "\x1b[33m";
    const char* red = "\x1b[31m";
    const char* w_on_r = "\x1b[97;41m";
    const char* blk_on_br = "\x1b[30;101m";
    const char* dimgray = "\x1b[38;5;240m";
    const char* white = "\x1b[37m";
    const char* bwhite = "\x1b[97m";
    const char* red_error = "\x1b[38;5;160m";     // crimson
    const char* red_fatal = "\x1b[38;5;196m";     // bright red
    const char* bgreen = "\x1b[92m";              // bright green
    const char* green_lime = "\x1b[38;5;46m";     // vivid lime
    const char* orange = "\x1b[38;5;208m";        // standard orange
    const char* orange_bright = "\x1b[38;5;214m"; // bright orange
    const char* orange_red    = "\x1b[38;5;202m"; // deep orange/red-orange

    constexpr uint32_t Bit(Level level) {
        return 1u << static_cast<uint32_t>(level);
    }

    void EnableLevel(Level level) {
        g_levelMask.fetch_or(Bit(level), std::memory_order_relaxed);
    }

    void DisableLevel(Level level) {
        g_levelMask.fetch_and(~Bit(level), std::memory_order_relaxed);
    }

    bool IsEnabled(Level L) {
        return (g_levelMask.load(std::memory_order_relaxed) & Bit(L)) != 0;
    }

    MessageStream Message(Level level) {
        return MessageStream(level);
    }

    #if defined(_WIN32)
    void EnableVT() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE) return;
        DWORD m = 0; if (!GetConsoleMode(h, &m)) return;
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        g_vtInitialized = true;
    }
    #else
    void EnableVT() {
        // Nothing as of yet
    }
    #endif

    const char* Color(Level level) {
        switch (level) {
            case Level::INIT:       return yellow;
            case Level::DEBUG:      return cyan;
            case Level::ERROR:      return red_error;
            case Level::WARNING:    return orange;
            case Level::FATAL:      return red_fatal;
			case Level::TODO:       return green_lime;
            case Level::FUNCTION:   return green_lime; // bblue;
            case Level::SUPPORT:    return green_lime;
            case Level::EDITOR:     return green_lime;
            default:                return bwhite;
        }
    }

    const char* Name(Level level) {
        switch (level) {
            case Level::INIT:       return "INIT";
            case Level::DEBUG:      return "DEBUG";
            case Level::ERROR:      return "ERROR";
            case Level::WARNING:    return "WARNING";
            case Level::FATAL:      return "FATAL";
            case Level::SUPPORT:    return "SUPPORT";
            case Level::EDITOR:    return  "EDITOR";
        }
        return "UNDEFINED";
    }

    bool VTInitialized() {
        return g_vtInitialized;
    }

    void MessageOld(const std::string& message, Level Level) {
        if (!IsEnabled(Level)) return;
        if (!VTInitialized()) EnableVT();

        std::cout << Color(Level) << "[" << Name(Level) << "] " << message << colorReset << "\n";
    }

    MessageStream::MessageStream(Level level)
        : m_level(level), m_enabled(IsEnabled(level)) {
    }

    MessageStream::~MessageStream() {
        if (!m_enabled || m_moved) return;
        if (!VTInitialized()) EnableVT();

        const std::string message = m_ss.str();
        if (message.empty()) return;

        const std::string labelText = std::string("[") + Name(m_level) + "] ";
        const std::string indent(labelText.size(), ' ');

        // Build once, keeping color active until the final reset
        std::ostringstream output;
        output << Color(m_level) << labelText;

        for (size_t i = 0; i < message.size(); ++i) {
            char ch = message[i];
            output << ch;
            if (ch == '\n' && i + 1 < message.size()) {
                output << indent;
            }
        }

        // Ensure one trailing newline and reset colors
        if (message.empty() || message.back() != '\n') output << '\n';
        output << colorReset;

        std::cout << output.str();
    }

    MessageStream::MessageStream(MessageStream&& rhs) noexcept
        : m_level(rhs.m_level),
        m_enabled(rhs.m_enabled),
        m_moved(false),
        m_ss(std::move(rhs.m_ss)) {
        rhs.m_moved = true;
    }

    MessageStream& MessageStream::operator=(MessageStream&& rhs) noexcept {
        if (this == &rhs) return *this;
        m_level = rhs.m_level;
        m_enabled = rhs.m_enabled;
        m_ss = std::move(rhs.m_ss);
        m_moved = false;
        rhs.m_moved = true;
        return *this;
    }
}
