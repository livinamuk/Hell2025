#pragma once

namespace Hell::InputMulti {
    struct MouseState {
        bool detected = false;
        bool leftMouseDown = false;
        bool rightMouseDown = false;
        bool leftMousePressed = false;
        bool rightMousePressed = false;
        bool leftMouseDownLastFrame = false;
        bool rightMouseDownLastFrame = false;
        bool leftMouseConsumed = false;
        bool rightMouseConsumed = false;
        int xoffset = 0;
        int yoffset = 0;
    };

    struct KeyboardState {
        bool keyPressed[372];
        bool keyDown[372];
        bool keyDownLastFrame[372];
        bool keyRepeat[372];
        bool consumed[372];
        float keyDownTime[372];
        float keyRepeatTime[372];
        float nextRepeatAt[372];
    };

    void Init();
    void ResetState();
    void Update();
    bool LeftMouseDown(int index);
    bool RightMouseDown(int index);
    bool LeftMousePressed(int index);
    bool RightMousePressed(int index);
    int GetMouseXOffset(int index);
    int GetMouseYOffset(int index);
    bool KeyPressed(int keyboardIndex, int mouseIndex, unsigned int keyCode, bool allowKeyRepeat = false);
    bool KeyDown(int keyboardIndex, int mouseIndex, unsigned int keyCode);
    void ConsumeKey(int keyboardIndex, int mouseIndex, unsigned int keyCode);
    void ResetMouseOffsets();
    void ClearKeyStates();
}
