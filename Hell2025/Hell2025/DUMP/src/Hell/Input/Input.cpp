#include "Input.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace Hell::Input {
    bool g_keyPressed[372];
    bool g_keyDown[372];
    bool g_keyDownLastFrame[372];
    double g_mouseX = 0;
    double g_mouseY = 0;
    double g_mouseOffsetX = 0;
    double g_mouseOffsetY = 0;
    int g_mouseWheelValue = 0;
    int g_sensitivity = 100;
    bool g_cursorVisible = false;
    bool g_mouseWheelUp = false;
    bool g_mouseWheelDown = false;
    bool g_leftMouseDown = false;
    bool g_rightMouseDown = false;
    bool g_leftMousePressed = false;
    bool g_rightMousePressed = false;
    bool g_leftMouseDownLastFrame = false;
    bool g_rightMouseDownLastFrame = false;
    bool g_middleMouseDown = false;
    bool g_middleMousePressed = false;
    bool g_middleMouseDownLastFrame = false;
    bool g_preventRightMouseHoldTillNextClick = false;
    int g_mouseScreenX = 0;
    int g_mouseScreenY = 0;
    int g_scrollWheelYOffset = 0;
    int g_mouseXPreviousFrame = 0;
    int g_mouseYPreviousFrame = 0;
    GLFWwindow* g_window;

    void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    void Init(void* glfwWindow) {
        Logging::Init() << "Initialized input";

        double x, y;
        g_window = static_cast<GLFWwindow*>(glfwWindow);
        glfwSetScrollCallback(g_window, MouseScrollCallback);
        glfwGetCursorPos(g_window, &x, &y);
        DisableCursor();
        g_mouseOffsetX = x;
        g_mouseOffsetY = y;
        g_mouseX = x;
        g_mouseY = y;
    }

    void Update() {
        // Wheel
        g_mouseWheelUp = false;
        g_mouseWheelDown = false;
        g_mouseWheelValue = g_scrollWheelYOffset;
        if (g_mouseWheelValue < 0)
            g_mouseWheelDown = true;
        if (g_mouseWheelValue > 0)
            g_mouseWheelUp = true;
        g_scrollWheelYOffset = 0;

        // Keyboard
        for (int i = 32; i < 349; i++) {
            // down
            if (glfwGetKey(g_window, i) == GLFW_PRESS)
                g_keyDown[i] = true;
            else
                g_keyDown[i] = false;

            // press
            if (g_keyDown[i] && !g_keyDownLastFrame[i])
                g_keyPressed[i] = true;
            else
                g_keyPressed[i] = false;
            g_keyDownLastFrame[i] = g_keyDown[i];
        }

        // Mouse
        g_mouseXPreviousFrame = g_mouseX;
        g_mouseYPreviousFrame = g_mouseY;
        double x, y;
        glfwGetCursorPos(g_window, &x, &y);
        g_mouseOffsetX = x - g_mouseX;
        g_mouseOffsetY = y - g_mouseY;
        g_mouseX = x;
        g_mouseY = y;
        g_cursorVisible = glfwGetInputMode(g_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;

        // Left mouse down/pressed
        g_leftMouseDown = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT);
        if (g_leftMouseDown == GLFW_PRESS && !g_leftMouseDownLastFrame)
            g_leftMousePressed = true;
        else
            g_leftMousePressed = false;
        g_leftMouseDownLastFrame = g_leftMouseDown;

        // Right mouse down/pressed
        g_rightMouseDown = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_RIGHT);
        if (g_rightMouseDown == GLFW_PRESS && !g_rightMouseDownLastFrame)
            g_rightMousePressed = true;
        else
            g_rightMousePressed = false;
        g_rightMouseDownLastFrame = g_rightMouseDown;

        if (g_rightMousePressed)
            g_preventRightMouseHoldTillNextClick = false;

        // Middle button
        g_middleMouseDown = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_MIDDLE);
        if (g_middleMouseDown == GLFW_PRESS && !g_middleMouseDownLastFrame)
            g_middleMousePressed = true;
        else
            g_middleMousePressed = false;
        g_middleMouseDownLastFrame = g_middleMouseDown;
    }

    void ClearKeyStates() {
        for (int i = 32; i < 349; i++) {
            g_keyDown[i] = false;
            g_keyPressed[i] = false;
            g_keyDownLastFrame[i] = false;
        }
    }

    bool KeyPressed(unsigned int keycode) {
        return g_keyPressed[keycode];
    }

    bool KeyDown(unsigned int keycode) {
        return g_keyDown[keycode];
    }

    float GetMouseOffsetX() {
        return (float)g_mouseOffsetX;
    }

    float GetMouseOffsetY() {
        return (float)g_mouseOffsetY;
    }

    bool LeftMouseDown() {
        return g_leftMouseDown;
    }

    bool MiddleMouseDown() {
        return g_middleMouseDown;
    }

    bool RightMouseDown() {
        return g_rightMouseDown && !g_preventRightMouseHoldTillNextClick;
    }

    bool LeftMousePressed() {
        return g_leftMousePressed;
    }

    bool MiddleMousePressed() {
        return g_middleMousePressed;
    }

    bool RightMousePressed() {
        return g_rightMousePressed;
    }

    bool MouseWheelDown() {
        return g_mouseWheelDown;
    }

    int GetMouseWheelValue() {
        return g_mouseWheelValue;
    }

    bool MouseWheelUp() {
        return g_mouseWheelUp;
    }

    void PreventRightMouseHold() {
        g_preventRightMouseHoldTillNextClick = true;
    }

    int GetMouseX() {
        return (int)g_mouseX;
    }

    int GetMouseY() {
        return (int)g_mouseY;
    }

    int GetMouseXPreviousFrame() {
        return (int)g_mouseXPreviousFrame;
    }

    int GetMouseYPreviousFrame() {
        return (int)g_mouseYPreviousFrame;
    }

    void DisableCursor() {
        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void HideCursor() {
        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    void ShowCursor() {
        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    bool CursorVisible() {
        return g_cursorVisible;
    }
    
    void CenterMouseCursor() {
        SetCursorPosition(Hell::BackEnd::GetCurrentWindowWidth() / 2, Hell::BackEnd::GetCurrentWindowHeight() / 2);
    }

    int GetCursorScreenX() {
        return g_mouseScreenX;
    }

    int GetCursorScreenY() {
        return g_mouseScreenY;
    }

    void SetCursorPosition(int x, int y) {
        glfwSetCursorPos(g_window, static_cast<double>(x), static_cast<double>(y));
        g_mouseX = x;
        g_mouseY = y;
        g_mouseOffsetX = 0;
        g_mouseOffsetY = 0;
        g_mouseXPreviousFrame = x;
        g_mouseYPreviousFrame = y;
    }

    void MouseScrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
        g_scrollWheelYOffset = (int)yoffset;
    }
}
