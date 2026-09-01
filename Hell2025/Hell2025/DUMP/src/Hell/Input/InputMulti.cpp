#include "InputMulti.h"

#include "Hell/Logging.h"
#include "Hell/Backend/Integration/GLFW.h"
#include "Hell/Input/keycodes.h"

#include <cstring>
#include <iostream>
#include <vector>

#define NOMINMAX
#include <Windows.h>
#ifdef _MSC_VER
#undef GetObject
#endif

namespace Hell::InputMulti {
    MouseState g_mouseStates[4];
    KeyboardState g_keyboardStates[4];
    std::vector<HANDLE> g_mouseHandles;
    std::vector<HANDLE> g_keyboardHandles;
    const USHORT HID_USAGE_GENERIC_MOUSE = 0x02;
    const USHORT HID_USAGE_GENERIC_KEYBOARD = 0x06;

    int GetHandleIndex(std::vector<HANDLE>* handleVector, HANDLE handle) {
        for (int i = 0; i < handleVector->size(); i++) {
            if ((*handleVector)[i] == handle) {
                return i;
            }
        }
        handleVector->push_back(handle);
        return (int)handleVector->size() - 1;
    }

    LRESULT CALLBACK targetWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_INPUT) {
            UINT dataSize = 0;
            // First call to get data size
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

            if (dataSize > 0)
            {
                RAWINPUT raw = RAWINPUT();
                // Second call to get the actual data
                if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
                {
                    // Mice
                    if (raw.header.dwType == RIM_TYPEMOUSE)
                    {
                        int mouseID = GetHandleIndex(&g_mouseHandles, raw.header.hDevice);
                        if (mouseID >= 4) return 0;

                        switch (raw.data.mouse.ulButtons) {
                        case RI_MOUSE_LEFT_BUTTON_DOWN: g_mouseStates[mouseID].leftMouseDown = true;	break;
                        case RI_MOUSE_LEFT_BUTTON_UP: g_mouseStates[mouseID].leftMouseDown = false; break;
                        case RI_MOUSE_RIGHT_BUTTON_DOWN: g_mouseStates[mouseID].rightMouseDown = true; break;
                        case RI_MOUSE_RIGHT_BUTTON_UP: g_mouseStates[mouseID].rightMouseDown = false; break;
                        }

                        // Wheel change values are device-dependent. Check RAWMOUSE docs for details.
                        if (raw.data.mouse.usButtonData != 0) {
                            //	cout << "MOUSE " << mouseID << ": WHEEL CHANGE " << raw.data.mouse.usButtonData << endl;
                        }

                        g_mouseStates[mouseID].xoffset -= raw.data.mouse.lLastX;
                        g_mouseStates[mouseID].yoffset += raw.data.mouse.lLastY;
                    }
                    // Keyboard
                    else if (raw.header.dwType == RIM_TYPEKEYBOARD)
                    {
                        int keyboardID = GetHandleIndex(&g_keyboardHandles, raw.header.hDevice);
                        auto keycode = raw.data.keyboard.VKey;
                        if (keyboardID >= 4) return 0;

                        //std::cout << keycode << "\n";

                        switch (raw.data.keyboard.Flags) {
                        case RI_KEY_MAKE: g_keyboardStates[keyboardID].keyDown[keycode] = true; break;
                        case RI_KEY_BREAK: g_keyboardStates[keyboardID].keyDown[keycode] = false; break;
                        }
                    }
                }
            }
            return 0;
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    bool RegisterDeviceOfType(USHORT type, HWND eventWindow) {
        RAWINPUTDEVICE rid = {};
        rid.usUsagePage = 0x01;
        rid.usUsage = type;
        rid.dwFlags = 0;
        rid.hwndTarget = eventWindow;
        return RegisterRawInputDevices(&rid, 1, sizeof(rid));
    }

    void Init() {
        HINSTANCE hInstance = GetModuleHandle(NULL);
        WNDCLASS windowClass = {};
        windowClass.lpfnWndProc = targetWindowProc;
        windowClass.hInstance = hInstance;
        windowClass.lpszClassName = TEXT("InputWindow");

        if (!RegisterClass(&windowClass)) {
            Logging::Fatal() << "Failed to register InputMulti window class";
            return;
        }

        HWND eventWindow = CreateWindowEx(0, windowClass.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
        if (!eventWindow) {
            Logging::Fatal() << "Failed to register InputMulti window class";
            return;
        }
        else {
            Logging::Init() << "Multi mouse/keyboard support";
        }

        RegisterDeviceOfType(HID_USAGE_GENERIC_MOUSE, eventWindow);
        RegisterDeviceOfType(HID_USAGE_GENERIC_KEYBOARD, eventWindow);
    }

    void ResetState() {
        std::memset(g_keyboardStates, 0, sizeof(g_keyboardStates));
        std::memset(g_mouseStates, 0, sizeof(g_mouseStates));
    }

    void Update() {
        const float deltaTime = 1.0f / 60.0f;

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        for (MouseState& state : g_mouseStates) {
            // Left mouse down/pressed
            if (state.leftMouseDown && !state.leftMouseDownLastFrame) {
                state.leftMousePressed = true;
            }
            else {
                state.leftMousePressed = false;
            }
            state.leftMouseDownLastFrame = state.leftMouseDown;

            // Right mouse down/pressed
            if (state.rightMouseDown && !state.rightMouseDownLastFrame) {
                state.rightMousePressed = true;
            }
            else {
                state.rightMousePressed = false;
            }
            state.rightMouseDownLastFrame = state.rightMouseDown;
        }

        const float initialDelay = 0.36f;
        const float repeatInterval = 0.08f;

        // Key press/down/repeat
        for (KeyboardState& state : g_keyboardStates) {
            for (int i = 0; i < 350; ++i) {
                state.consumed[i] = false;

                if (state.keyDown[i]) {
                    if (!state.keyDownLastFrame[i]) {
                        state.keyPressed[i] = true;
                        state.keyDownTime[i] = 0.0f;
                        state.keyRepeatTime[i] = 0.0f;
                        state.nextRepeatAt[i] = initialDelay;
                    }
                    else {
                        state.keyPressed[i] = false;
                        state.keyDownTime[i] += deltaTime;
                    }

                    // Fire when reached ored pass the scheduled time
                    bool repeatPulse = false;
                    while (state.keyDownTime[i] >= state.nextRepeatAt[i]) {
                        repeatPulse = true;
                        state.nextRepeatAt[i] += repeatInterval; // Catch up if a long frame
                    }

                    state.keyRepeat[i] = state.keyPressed[i] || repeatPulse;
                }
                else {
                    state.keyPressed[i] = false;
                    state.keyRepeat[i] = false;
                    state.keyDownTime[i] = 0.f;
                    state.keyRepeatTime[i] = 0.f;
                }

                state.keyDownLastFrame[i] = state.keyDown[i];
            }
        }

        // Out of window focus? then remove any detected input
        HWND activeWindow = GetActiveWindow();
        HWND myWindow = (HWND)Hell::BackEnd::GLFW::GetWin32Window();
        if ((void*)myWindow != (void*)activeWindow) {
            for (KeyboardState& state : g_keyboardStates) {
                for (int i = 0; i < 350; i++) {
                    state.keyPressed[i] = false;
                    state.keyDownLastFrame[i] = false;
                }
            }
            for (MouseState& state : g_mouseStates) {
                state.leftMousePressed = false;
                state.leftMouseDownLastFrame = false;
                state.rightMousePressed = false;
                state.rightMouseDownLastFrame = false;
            }
        }
    }

    void ClearKeyStates() {
        for (KeyboardState& state : g_keyboardStates) {
            for (int i = 0; i < 350; ++i) {
                state.keyPressed[i] = false;
                state.keyDown[i] = false;
                state.keyDownLastFrame[i] = false;
                state.keyDownTime[i] = 0.0f;
                state.keyRepeatTime[i] = 0.0f;
                state.nextRepeatAt[i] = 0.0f;
            }
        }
    }

    void ResetMouseOffsets() {
        for (MouseState& state : g_mouseStates) {
            state.xoffset = 0;
            state.yoffset = 0;
        }
    }

    int GetMouseYOffset(int index) {
        if (index < 0 || index >= 4)
            return 0;
        else
            return g_mouseStates[index].yoffset;
    }

    bool LeftMouseDown(int index) {
        if (index < 0 || index >= 4)
            return false;
        else
            return g_mouseStates[index].leftMouseDown;
    }

    bool RightMouseDown(int index) {
        if (index < 0 || index >= 4)
            return false;
        else
            return g_mouseStates[index].rightMouseDown;
    }

    bool LeftMousePressed(int index) {
        if (index < 0 || index >= 4)
            return false;
        else
            return g_mouseStates[index].leftMousePressed;
    }

    bool RightMousePressed(int index) {
        if (index < 0 || index >= 4)
            return false;
        else
            return g_mouseStates[index].rightMousePressed;
    }

    int GetMouseXOffset(int index) {
        if (index < 0 || index >= 4)
            return 0;
        else
            return g_mouseStates[index].xoffset;
    }

    bool KeyDown(int keyboardIndex, int mouseIndex, unsigned int keyCode) {
        // Mouse
        if (keyCode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4 && !g_mouseStates[mouseIndex].leftMouseConsumed)
            return g_mouseStates[mouseIndex].leftMouseDown;
        else if (keyCode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4 && !g_mouseStates[mouseIndex].rightMouseConsumed)
            return g_mouseStates[mouseIndex].rightMouseDown;

        // Keyboard
        if (keyboardIndex >= 0 && keyboardIndex < 4 && keyCode >= 0 && keyCode < 350) {
            return g_keyboardStates[keyboardIndex].keyDown[keyCode] && !g_keyboardStates[keyboardIndex].consumed[keyCode];
        }

        return false;
    }

    void ConsumeKey(int keyboardIndex, int mouseIndex, unsigned int keyCode) {
        // Mouse
        if (keyCode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4) {
            g_mouseStates[mouseIndex].leftMouseConsumed = true;
            return;
        }
        else if (keyCode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4) {
            g_mouseStates[mouseIndex].rightMouseConsumed = true;
            return;
        }

        // Keyboard
        if (keyboardIndex >= 0 && keyboardIndex < 4 && keyCode >= 0 && keyCode < 350) {
            g_keyboardStates[keyboardIndex].consumed[keyCode] = true;
            return;
        }
    }

    bool KeyPressed(int keyboardIndex, int mouseIndex, unsigned int keyCode, bool allowKeyRepeat) {
        // Mouse
        if (keyCode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4)
            return g_mouseStates[mouseIndex].leftMousePressed && !g_mouseStates[mouseIndex].leftMouseConsumed;
        if (keyCode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4)
            return g_mouseStates[mouseIndex].rightMousePressed && !g_mouseStates[mouseIndex].rightMouseConsumed;

        // Keyboard
        if (keyboardIndex >= 0 && keyboardIndex < 4 && keyCode >= 0 && keyCode < 350) {
            const KeyboardState& state = g_keyboardStates[keyboardIndex];
            if (state.consumed[keyCode]) {
                return false;
            }
            return allowKeyRepeat ? state.keyRepeat[keyCode] : state.keyPressed[keyCode];
        }

        return false;
    }
}
