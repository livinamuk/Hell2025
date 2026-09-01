#pragma once
#include <SDL3/SDL.h>

namespace Hell::InputMulti {
    void Init();
    void Update();
    void ResetState();
    void ProcessEvent(const SDL_Event& e);

    bool LeftMouseDown(int index);
    bool RightMouseDown(int index);
    bool LeftMousePressed(int index);
    bool RightMousePressed(int index);
    bool KeyPressed(int keyboardIndex, int mouseIndex, unsigned int keyCode, bool allowKeyRepeat = false);
    bool KeyDown(int keyboardIndex, int mouseIndex, unsigned int keyCode);
    void ResetMouseOffsets();
    void ClearKeyStates();

    int GetMouseXOffset(int index);
    int GetMouseYOffset(int index);
    int GetFirstAvailableKeyboard();
    int GetFirstAvailableMouse();

    void Test();
}
