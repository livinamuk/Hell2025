#include "GLFW.h"
#include <glad/gl.h>
#define NOMINMAX
#include <Windows.h>
#ifdef _MSC_VER
#undef GetObject
#endif
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <unordered_map>

namespace Hell::BackEnd::GLFW {
    GLFWwindow* g_window = NULL;
    WindowedMode g_windowedMode = WindowedMode::WINDOWED;
    GLFWmonitor* g_monitor;
    std::unordered_map<int, GLFWcursor*> g_cursorsPtrs;
    const GLFWvidmode* g_mode;
    bool g_forceCloseWindow = false;
    bool g_windowHasFocus = true;
    int g_windowedWidth = 0;
    int g_windowedHeight = 0;
    int g_fullscreenWidth = 0;
    int g_fullscreenHeight = 0;
    int g_currentWindowWidth = 0;
    int g_currentWindowHeight = 0;
    int g_currentCursor = GLFW_ARROW_CURSOR;

    void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    //void window_focus_callback(GLFWwindow* window, int focused);

    bool Init(API api, WindowedMode windowedMode) {
        glfwInit();
        glfwSetErrorCallback([](int error, const char* description) { std::cout << "GLFW Error (" << std::to_string(error) << "): " << description << "\n"; });

        if (api == API::OPENGL) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true); 
            glfwWindowHint(GLFW_SAMPLES, 0);
        }
        else if (api == API::VULKAN) {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        }
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);

        // Resolution and window size
        g_monitor = glfwGetPrimaryMonitor();
        g_mode = glfwGetVideoMode(g_monitor);
        glfwWindowHint(GLFW_RED_BITS, g_mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, g_mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, g_mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, g_mode->refreshRate);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        g_fullscreenWidth = g_mode->width;
        g_fullscreenHeight = g_mode->height;
        g_windowedWidth = g_fullscreenWidth * 0.75f;
        g_windowedHeight = g_fullscreenHeight * 0.75f;

        // Create window
        g_windowedMode = windowedMode;
        if (g_windowedMode == WindowedMode::WINDOWED) {
            g_currentWindowWidth = g_windowedWidth;
            g_currentWindowHeight = g_windowedHeight;
            g_window = glfwCreateWindow(g_windowedWidth, g_windowedHeight, "Unloved", NULL, NULL);
            glfwSetWindowPos(g_window, 0, 0);
        }
        else if (windowedMode == WindowedMode::FULLSCREEN) {
            g_currentWindowWidth = g_fullscreenWidth;
            g_currentWindowHeight = g_fullscreenHeight;
            g_window = glfwCreateWindow(g_fullscreenWidth, g_fullscreenHeight, "Unloved", g_monitor, NULL);
        }
        g_windowedMode = windowedMode;
        if (g_window == NULL) {
            std::cout << "GLFWIntegration() Failed to create window\n";
            glfwTerminate();
            return false;
        }
        glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
        //glfwSetWindowFocusCallback(g_window, window_focus_callback);

        g_cursorsPtrs[GLFW_ARROW_CURSOR] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_cursorsPtrs[GLFW_IBEAM_CURSOR] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        g_cursorsPtrs[GLFW_CROSSHAIR_CURSOR] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
        g_cursorsPtrs[GLFW_HAND_CURSOR] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        g_cursorsPtrs[GLFW_HRESIZE_CURSOR] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        g_cursorsPtrs[GLFW_VRESIZE_CURSOR] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);

        return true;
    }

    void SetCursor(int cursor) {
        g_currentCursor = cursor;
        glfwSetCursor(g_window, g_cursorsPtrs[cursor]);
    }

    void* GetWin32Window() {
        return glfwGetWin32Window(g_window);
    }

    void MakeContextCurrent() {
        glfwMakeContextCurrent(g_window);
    }

    bool CreateSurface(void* surface) {
        // Vulkan surface creation will be rewired with the new backend.
        return false;
    }

    void BeginFrame(API api) {
        glfwPollEvents();

        // Query window focus
        int focused = glfwGetWindowAttrib(g_window, GLFW_FOCUSED);
        g_windowHasFocus = (focused != 0);
    }

    void EndFrame(API api) {
        if (api == API::OPENGL) {
            glfwSwapBuffers(g_window);
        }
        else if (api == API::VULKAN) {
            // Nothing required
        }
    }

    void Destroy() {
        if (GetWindowedMode() == WindowedMode::FULLSCREEN) {
            ToggleFullscreen();
        }
        glfwDestroyCursor(g_cursorsPtrs[GLFW_ARROW_CURSOR]);
        glfwDestroyCursor(g_cursorsPtrs[GLFW_IBEAM_CURSOR]);
        glfwDestroyCursor(g_cursorsPtrs[GLFW_CROSSHAIR_CURSOR]);
        glfwDestroyCursor(g_cursorsPtrs[GLFW_HAND_CURSOR]);
        glfwDestroyCursor(g_cursorsPtrs[GLFW_HRESIZE_CURSOR]);
        glfwDestroyCursor(g_cursorsPtrs[GLFW_VRESIZE_CURSOR]);
        glfwTerminate();
    }

    void SetWindowedMode(const WindowedMode& windowedMode) {
        g_windowedMode = windowedMode;

        if (windowedMode == WindowedMode::WINDOWED) {
            g_currentWindowWidth = g_windowedWidth;
            g_currentWindowHeight = g_windowedHeight;
        }
        else if (windowedMode == WindowedMode::FULLSCREEN) {
            g_currentWindowWidth = g_fullscreenWidth - 1;
            g_currentWindowHeight = g_fullscreenHeight - 1;
        }

        glfwSetWindowMonitor(g_window, nullptr, 0, 0, g_currentWindowWidth, g_currentWindowHeight, g_mode->refreshRate);
        glfwSetWindowPos(g_window, 0, 0);
    }

    void ToggleFullscreen() {
        if (g_windowedMode == WindowedMode::WINDOWED) {
            SetWindowedMode(WindowedMode::FULLSCREEN);
        }
        else {
            SetWindowedMode(WindowedMode::WINDOWED);
        }
    }

    void ForceCloseWindow() {
        g_forceCloseWindow = true;
    }

    bool WindowHasFocus() {
        return g_windowHasFocus;
    }

    bool WindowHasNotBeenForceClosed() {
        return !g_forceCloseWindow;
    }

    void* GetWindowPointer() {
        return g_window;
    }

    int GetWindowedWidth() {
        return g_windowedWidth;
    }

    int GetWindowedHeight() {
        return g_windowedHeight;
    }

    int GetFullScreenWidth() {
        return g_fullscreenWidth;
    }

    int GetFullScreenHeight() {
        return g_fullscreenHeight;
    }

    int GetCurrentWindowWidth() {
        if (g_window) glfwGetWindowSize(g_window, &g_currentWindowWidth, &g_currentWindowHeight);
        return g_currentWindowWidth;
    }

    int GetCurrentWindowHeight() {
        if (g_window) glfwGetWindowSize(g_window, &g_currentWindowWidth, &g_currentWindowHeight);
        return g_currentWindowHeight;
    }

    int GetDrawableWidth() {
        int width = g_currentWindowWidth;
        int height = g_currentWindowHeight;
        if (g_window) glfwGetFramebufferSize(g_window, &width, &height);
        return width;
    }

    int GetDrawableHeight() {
        int width = g_currentWindowWidth;
        int height = g_currentWindowHeight;
        if (g_window) glfwGetFramebufferSize(g_window, &width, &height);
        return height;
    }

    bool WindowIsOpen() {
        return !(glfwWindowShouldClose(g_window) || g_forceCloseWindow);
    }

    bool WindowIsMinimized() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(g_window, &width, &height);
        return (width == 0 || height == 0);
    }

    const WindowedMode& GetWindowedMode() {
        return g_windowedMode;
    }

    const std::vector<const char*> GetRequiredInstanceExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions) {
            std::cerr << "GLFWIntegration::GetRequiredInstanceExtensions() failed to get required instance extensions\n";
            return {};
        }
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        // Debug: Print the required extensions
        //std::cout << "Required extensions:\n";
        //for (const auto& ext : extensions) {
        //    std::cout << " - " << ext << "\n";
        //}
        return extensions;
    }

    void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
        // Nothing as of yet
    }
}
