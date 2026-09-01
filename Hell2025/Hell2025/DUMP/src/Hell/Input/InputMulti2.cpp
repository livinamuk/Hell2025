#include "InputMulti2.h"

#include "Hell/Logging.h"
#include "Hell/Input/keycodes.h"

#include <cstring>
#include <unordered_map>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <iostream>

namespace Hell::InputMulti {

    struct MouseState {
        int xoffset = 0;
        int yoffset = 0;
        bool leftMouseDown = false;
        bool rightMouseDown = false;
        bool leftMousePressed = false;
        bool rightMousePressed = false;
        bool leftMouseDownLastFrame = false;
        bool rightMouseDownLastFrame = false;
    };

    struct KeyboardState {
        bool keyDown[350];
        bool keyPressed[350];
        bool keyRepeat[350];
        bool keyDownLastFrame[350];
        float keyDownTime[350];
        float keyRepeatTime[350];
        float nextRepeatAt[350];
    };

    struct KnownMouse {
        SDL_MouseID id = 0;
        std::string name;
        bool assigned = false;
        int stateSlot = -1;
    };

    struct KnownKeyboard {
        SDL_KeyboardID id = 0;
        std::string name;
        bool assigned = false;
        int stateSlot = -1;
    };

    bool g_initialized = false;
    MouseState g_mouseStates[4];
    KeyboardState g_keyboardStates[4];
    std::unordered_map<SDL_MouseID, int> g_mouseIdToIndex;
    std::unordered_map<SDL_KeyboardID, int> g_keyboardIdToIndex;
    std::vector<KnownMouse> g_knownMice;
    std::vector<KnownKeyboard> g_knownKeyboards;
    std::string g_initLog = "";

    int GetOrCreateMouseIndex(SDL_MouseID id);
    int GetOrCreateKeyboardIndex(SDL_KeyboardID id);
    int SDLMouseButtonToHellKey(Uint8 button);
    int SDLScancodeToHellKey(SDL_Scancode sc);
    void MarkKeyboardAssigned(int stateSlot);
    void MarkMouseAssigned(int stateSlot);

    void Test() {

        bool running = true;
        while (running) {
            
            Update();

            if (g_keyboardStates[0].keyDown[HELL_KEY_SPACE]) {
                std::cout << "0 space\n";
            }
            if (g_keyboardStates[1].keyDown[HELL_KEY_SPACE]) {
                std::cout << "1 space\n";
            }
            if (g_keyboardStates[0].keyPressed[HELL_KEY_SPACE]) {
                std::cout << "0 space pressed\n";
            }
            if (g_keyboardStates[1].keyPressed[HELL_KEY_SPACE]) {
                std::cout << "1 space pressed\n";
            }
        }
    }

    void Init() {
        if (g_initialized) return;

        g_initLog = "InputMulti\n";

        ResetState();
        
        int mouseCount = 0;
        SDL_MouseID* mice = SDL_GetMice(&mouseCount);
        if (mice) {
            for (int i = 0; i < mouseCount; ++i) {
                SDL_MouseID id = mice[i];
                const char* name = SDL_GetMouseNameForID(id);
                int slot = GetOrCreateMouseIndex(id);
                KnownMouse km;
                km.id = id;
                km.name = name ? name : "unknown";
                km.assigned = (slot >= 0);
                km.stateSlot = slot;
                g_knownMice.push_back(km);
                g_initLog += "- found mouse '" + km.name + "' id=" + std::to_string((int)id) + " slot=" + std::to_string(slot) + "\n";
            }
            SDL_free(mice);
        }

        int keyboardCount = 0;
        SDL_KeyboardID* keyboards = SDL_GetKeyboards(&keyboardCount);
        if (keyboards) {
            for (int i = 0; i < keyboardCount; ++i) {
                SDL_KeyboardID id = keyboards[i];
                const char* name = SDL_GetKeyboardNameForID(id);
                std::string nameStr = name ? name : "unknown";
                bool matchesMouse = false;
                for (const auto& m : g_knownMice) {
                    if (m.name == nameStr) {
                        matchesMouse = true;
                        break;
                    }
                }
                if (matchesMouse) {
                    //g_initLog += "- found keyboard '" + nameStr + "' id=" + std::to_string((int)id) + " ignored (mouse name match)\n";
                    continue;
                }
                int slot = GetOrCreateKeyboardIndex(id);
                KnownKeyboard kk;
                kk.id = id;
                kk.name = nameStr;
                kk.assigned = (slot >= 0);
                kk.stateSlot = slot;
                g_knownKeyboards.push_back(kk);
                g_initLog += "- found keyboard '" + nameStr + "' id=" + std::to_string((int)id) + " slot=" + std::to_string(slot) + "\n";
            }
            SDL_free(keyboards);
        }

        g_initialized = true;

        Logging::Init() << g_initLog;
    }

    void Update() {
        const float deltaTime = 1.0f / 60.0f;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ProcessEvent(e);
        }

        for (int i = 0; i < 4; ++i) {
            MouseState& state = g_mouseStates[i];
            if (state.leftMouseDown && !state.leftMouseDownLastFrame) {
                state.leftMousePressed = true;
            }
            else {
                state.leftMousePressed = false;
            }
            state.leftMouseDownLastFrame = state.leftMouseDown;

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

        for (int k = 0; k < 4; ++k) {
            KeyboardState& state = g_keyboardStates[k];
            for (int i = 0; i < 350; ++i) {
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

                    bool repeatPulse = false;
                    while (state.keyDownTime[i] >= state.nextRepeatAt[i]) {
                        repeatPulse = true;
                        state.nextRepeatAt[i] += repeatInterval;
                    }
                    state.keyRepeat[i] = state.keyPressed[i] || repeatPulse;
                }
                else {
                    state.keyPressed[i] = false;
                    state.keyRepeat[i] = false;
                    state.keyDownTime[i] = 0.0f;
                    state.keyRepeatTime[i] = 0.0f;
                }

                state.keyDownLastFrame[i] = state.keyDown[i];
            }
        }


        if (g_keyboardStates[0].keyPressed[HELL_KEY_SPACE]) {
            std::cout << "0 space pressed\n";
        }
        if (g_keyboardStates[1].keyPressed[HELL_KEY_SPACE]) {
            std::cout << "1 space pressed\n";
        }

        if (g_keyboardStates[0].keyDown[HELL_KEY_SPACE]) {
            std::cout << "0 space down\n";
        }
        if (g_keyboardStates[1].keyDown[HELL_KEY_SPACE]) {
            std::cout << "1 space down\n";
        }

        if (g_keyboardStates[1].keyPressed[HELL_KEY_E]) {
            std::cout << "e\n";
        }

        if (KeyPressed(0, 0, HELL_KEY_W, false)) {
            std::cout << "0: w pressed\n";
        }
        if (KeyPressed(1, 0, HELL_KEY_W, false)) {
            std::cout << "1: w pressed\n";
        }
    }

    void ProcessEvent(const SDL_Event& e) {
        switch (e.type) {
            // Lose window focus
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                ResetState();
                break;

            // Keyboard added
            case SDL_EVENT_KEYBOARD_ADDED: {
                SDL_KeyboardID id = e.kdevice.which;
                GetOrCreateKeyboardIndex(id);
                break;
            }

            // Keyboard removed
            case SDL_EVENT_KEYBOARD_REMOVED: {
                SDL_KeyboardID id = e.kdevice.which;
                auto it = g_keyboardIdToIndex.find(id);
                if (it != g_keyboardIdToIndex.end()) {
                    int idx = it->second;
                    std::memset(&g_keyboardStates[idx], 0, sizeof(KeyboardState));
                    g_keyboardIdToIndex.erase(it);
                }
                break;
            }

            // Keyboard input
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                const SDL_KeyboardEvent* ke = &e.key;
                int idx = GetOrCreateKeyboardIndex(ke->which);
                if (idx < 0) break;

                int hellKey = SDLScancodeToHellKey(ke->scancode);
                if (hellKey < 0 || hellKey >= 350) break;

                g_keyboardStates[idx].keyDown[hellKey] = ke->down;
                break;
            }

            // Mouse added
            case SDL_EVENT_MOUSE_ADDED: {
                SDL_MouseID id = e.mdevice.which;
                GetOrCreateMouseIndex(id);
                break;
            }
            
            // Mouse removed
            case SDL_EVENT_MOUSE_REMOVED: {
                SDL_MouseID id = e.mdevice.which;
                auto it = g_mouseIdToIndex.find(id);
                if (it != g_mouseIdToIndex.end()) {
                    int idx = it->second;
                    g_mouseStates[idx] = MouseState{};
                    g_mouseIdToIndex.erase(it);
                }
                break;
            }

            // Mouse movement
            case SDL_EVENT_MOUSE_MOTION: {
                int idx = GetOrCreateMouseIndex(e.motion.which);
                if (idx < 0) break;
                g_mouseStates[idx].xoffset += e.motion.xrel;
                g_mouseStates[idx].yoffset += e.motion.yrel;
                break;
            }

            // Mouse buttons
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                int idx = GetOrCreateMouseIndex(e.button.which);
                if (idx < 0) break;

                bool down = e.button.down;
                int hellMouse = SDLMouseButtonToHellKey(e.button.button);

                if (hellMouse == HELL_MOUSE_LEFT) {
                    g_mouseStates[idx].leftMouseDown = down;
                }
                else if (hellMouse == HELL_MOUSE_RIGHT) {
                    g_mouseStates[idx].rightMouseDown = down;
                }

                break;
            }

            default:
                break;
        }
    }

    void ResetState() {
        std::memset(g_keyboardStates, 0, sizeof(g_keyboardStates));
        std::memset(g_mouseStates, 0, sizeof(g_mouseStates));
    }

    int GetOrCreateMouseIndex(SDL_MouseID id) {
        auto it = g_mouseIdToIndex.find(id);
        if (it != g_mouseIdToIndex.end()) return it->second;
        if (g_mouseIdToIndex.size() >= 4) return -1;
        int newIndex = (int)g_mouseIdToIndex.size();
        g_mouseIdToIndex[id] = newIndex;
        g_mouseStates[newIndex] = MouseState{};
        return newIndex;
    }

    int GetOrCreateKeyboardIndex(SDL_KeyboardID id) {
        auto it = g_keyboardIdToIndex.find(id);
        if (it != g_keyboardIdToIndex.end()) return it->second;
        if (g_keyboardIdToIndex.size() >= 4) return -1;
        int newIndex = (int)g_keyboardIdToIndex.size();
        g_keyboardIdToIndex[id] = newIndex;
        std::memset(&g_keyboardStates[newIndex], 0, sizeof(KeyboardState));
        return newIndex;
    }

    int GetFirstAvailableMouse() {
        for (auto& m : g_knownMice) {
            if (!m.assigned && m.stateSlot >= 0) {
                m.assigned = true;
                return m.stateSlot;
            }
        }
        return -1;
    }

    int GetFirstAvailableKeyboard() {
        for (auto& k : g_knownKeyboards) {
            if (!k.assigned && k.stateSlot >= 0) {
                k.assigned = true;
                return k.stateSlot;
            }
        }
        return -1;
    }

    void MarkMouseAssigned(int stateSlot) {
        for (auto& m : g_knownMice) {
            if (m.stateSlot == stateSlot) {
                m.assigned = true;
                break;
            }
        }
    }

    void MarkKeyboardAssigned(int stateSlot) {
        for (auto& k : g_knownKeyboards) {
            if (k.stateSlot == stateSlot) {
                k.assigned = true;
                break;
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
        if (keyCode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4)
            return g_mouseStates[mouseIndex].leftMouseDown;
        else if (keyCode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4)
            return g_mouseStates[mouseIndex].rightMouseDown;

        // Keyboard
        if (keyboardIndex >= 0 && keyboardIndex < 4 && keyCode >= 0 && keyCode < 350) {
            return g_keyboardStates[keyboardIndex].keyDown[keyCode];
        }

        return false;
    }

    bool KeyPressed(int keyboardIndex, int mouseIndex, unsigned int keyCode, bool allowKeyRepeat) {
        // Mouse
        if (keyCode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4)
            return g_mouseStates[mouseIndex].leftMousePressed;
        if (keyCode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4)
            return g_mouseStates[mouseIndex].rightMousePressed;

        // Keyboard
        if (keyboardIndex >= 0 && keyboardIndex < 4 && keyCode >= 0 && keyCode < 350) {
            const KeyboardState& state = g_keyboardStates[keyboardIndex];
            return allowKeyRepeat ? state.keyRepeat[keyCode] : state.keyPressed[keyCode];
        }

        return false;
    }

    int SDLScancodeToHellKey(SDL_Scancode sc) {
        switch (sc) {
            case SDL_SCANCODE_A: return HELL_KEY_A;
            case SDL_SCANCODE_B: return HELL_KEY_B;
            case SDL_SCANCODE_C: return HELL_KEY_C;
            case SDL_SCANCODE_D: return HELL_KEY_D;
            case SDL_SCANCODE_E: return HELL_KEY_E;
            case SDL_SCANCODE_F: return HELL_KEY_F;
            case SDL_SCANCODE_G: return HELL_KEY_G;
            case SDL_SCANCODE_H: return HELL_KEY_H;
            case SDL_SCANCODE_I: return HELL_KEY_I;
            case SDL_SCANCODE_J: return HELL_KEY_J;
            case SDL_SCANCODE_K: return HELL_KEY_K;
            case SDL_SCANCODE_L: return HELL_KEY_L;
            case SDL_SCANCODE_M: return HELL_KEY_M;
            case SDL_SCANCODE_N: return HELL_KEY_N;
            case SDL_SCANCODE_O: return HELL_KEY_O;
            case SDL_SCANCODE_P: return HELL_KEY_P;
            case SDL_SCANCODE_Q: return HELL_KEY_Q;
            case SDL_SCANCODE_R: return HELL_KEY_R;
            case SDL_SCANCODE_S: return HELL_KEY_S;
            case SDL_SCANCODE_T: return HELL_KEY_T;
            case SDL_SCANCODE_U: return HELL_KEY_U;
            case SDL_SCANCODE_V: return HELL_KEY_V;
            case SDL_SCANCODE_W: return HELL_KEY_W;
            case SDL_SCANCODE_X: return HELL_KEY_X;
            case SDL_SCANCODE_Y: return HELL_KEY_Y;
            case SDL_SCANCODE_Z: return HELL_KEY_Z;
            case SDL_SCANCODE_1: return HELL_KEY_1;
            case SDL_SCANCODE_2: return HELL_KEY_2;
            case SDL_SCANCODE_3: return HELL_KEY_3;
            case SDL_SCANCODE_4: return HELL_KEY_4;
            case SDL_SCANCODE_5: return HELL_KEY_5;
            case SDL_SCANCODE_6: return HELL_KEY_6;
            case SDL_SCANCODE_7: return HELL_KEY_7;
            case SDL_SCANCODE_8: return HELL_KEY_8;
            case SDL_SCANCODE_9: return HELL_KEY_9;
            case SDL_SCANCODE_0: return HELL_KEY_0;
            case SDL_SCANCODE_SPACE: return HELL_KEY_SPACE;
            case SDL_SCANCODE_MINUS: return HELL_KEY_MINUS;
            case SDL_SCANCODE_EQUALS: return HELL_KEY_EQUAL;
            case SDL_SCANCODE_LEFTBRACKET: return HELL_KEY_LEFT_BRACKET;
            case SDL_SCANCODE_RIGHTBRACKET: return HELL_KEY_RIGHT_BRACKET;
            case SDL_SCANCODE_BACKSLASH: return HELL_KEY_BACKSLASH;
            case SDL_SCANCODE_SEMICOLON: return HELL_KEY_SEMICOLON;
            case SDL_SCANCODE_APOSTROPHE: return HELL_KEY_APOSTROPHE;
            case SDL_SCANCODE_GRAVE: return HELL_KEY_GRAVE_ACCENT;
            case SDL_SCANCODE_COMMA: return HELL_KEY_COMMA;
            case SDL_SCANCODE_PERIOD: return HELL_KEY_PERIOD;
            case SDL_SCANCODE_SLASH: return HELL_KEY_SLASH;
            case SDL_SCANCODE_ESCAPE: return HELL_KEY_ESCAPE;
            case SDL_SCANCODE_RETURN: return HELL_KEY_ENTER;
            case SDL_SCANCODE_TAB: return HELL_KEY_TAB;
            case SDL_SCANCODE_BACKSPACE: return HELL_KEY_BACKSPACE;
            case SDL_SCANCODE_INSERT: return HELL_KEY_INSERT;
            case SDL_SCANCODE_DELETE: return HELL_KEY_DELETE;
            case SDL_SCANCODE_RIGHT: return HELL_KEY_RIGHT;
            case SDL_SCANCODE_LEFT: return HELL_KEY_LEFT;
            case SDL_SCANCODE_DOWN: return HELL_KEY_DOWN;
            case SDL_SCANCODE_UP: return HELL_KEY_UP;
            case SDL_SCANCODE_PAGEUP: return HELL_KEY_PAGE_UP;
            case SDL_SCANCODE_PAGEDOWN: return HELL_KEY_PAGE_DOWN;
            case SDL_SCANCODE_HOME: return HELL_KEY_HOME;
            case SDL_SCANCODE_END: return HELL_KEY_END;
            case SDL_SCANCODE_CAPSLOCK: return HELL_KEY_CAPS_LOCK;
            case SDL_SCANCODE_SCROLLLOCK: return HELL_KEY_SCROLL_LOCK;
            case SDL_SCANCODE_NUMLOCKCLEAR: return HELL_KEY_NUM_LOCK;
            case SDL_SCANCODE_PRINTSCREEN: return HELL_KEY_PRINT_SCREEN;
            case SDL_SCANCODE_PAUSE: return HELL_KEY_PAUSE;
            case SDL_SCANCODE_F1: return HELL_KEY_F1;
            case SDL_SCANCODE_F2: return HELL_KEY_F2;
            case SDL_SCANCODE_F3: return HELL_KEY_F3;
            case SDL_SCANCODE_F4: return HELL_KEY_F4;
            case SDL_SCANCODE_F5: return HELL_KEY_F5;
            case SDL_SCANCODE_F6: return HELL_KEY_F6;
            case SDL_SCANCODE_F7: return HELL_KEY_F7;
            case SDL_SCANCODE_F8: return HELL_KEY_F8;
            case SDL_SCANCODE_F9: return HELL_KEY_F9;
            case SDL_SCANCODE_F10: return HELL_KEY_F10;
            case SDL_SCANCODE_F11: return HELL_KEY_F11;
            case SDL_SCANCODE_F12: return HELL_KEY_F12;
            case SDL_SCANCODE_LSHIFT: return HELL_KEY_LEFT_SHIFT;
            case SDL_SCANCODE_LCTRL: return HELL_KEY_LEFT_CONTROL;
            case SDL_SCANCODE_LALT: return HELL_KEY_LEFT_ALT;
            case SDL_SCANCODE_LGUI: return HELL_KEY_LEFT_SUPER;
            case SDL_SCANCODE_RSHIFT: return HELL_KEY_RIGHT_SHIFT;
            case SDL_SCANCODE_RCTRL: return HELL_KEY_RIGHT_CONTROL;
            case SDL_SCANCODE_RALT: return HELL_KEY_RIGHT_ALT;
            case SDL_SCANCODE_RGUI: return HELL_KEY_RIGHT_SUPER;
            case SDL_SCANCODE_KP_0: return HELL_KEY_KP_0;
            case SDL_SCANCODE_KP_1: return HELL_KEY_KP_1;
            case SDL_SCANCODE_KP_2: return HELL_KEY_KP_2;
            case SDL_SCANCODE_KP_3: return HELL_KEY_KP_3;
            case SDL_SCANCODE_KP_4: return HELL_KEY_KP_4;
            case SDL_SCANCODE_KP_5: return HELL_KEY_KP_5;
            case SDL_SCANCODE_KP_6: return HELL_KEY_KP_6;
            case SDL_SCANCODE_KP_7: return HELL_KEY_KP_7;
            case SDL_SCANCODE_KP_8: return HELL_KEY_KP_8;
            case SDL_SCANCODE_KP_9: return HELL_KEY_KP_9;
            case SDL_SCANCODE_KP_DECIMAL: return HELL_KEY_KP_DECIMAL;
            case SDL_SCANCODE_KP_DIVIDE: return HELL_KEY_KP_DIVIDE;
            case SDL_SCANCODE_KP_MULTIPLY: return HELL_KEY_KP_MULTIPLY;
            case SDL_SCANCODE_KP_MINUS: return HELL_KEY_KP_SUBTRACT;
            case SDL_SCANCODE_KP_PLUS: return HELL_KEY_KP_ADD;
            case SDL_SCANCODE_KP_ENTER: return HELL_KEY_KP_ENTER;
            default:
            return -1;
        }
    }

    int SDLMouseButtonToHellKey(Uint8 button) {
        switch (button) {
            case SDL_BUTTON_LEFT:  return HELL_MOUSE_LEFT;
            case SDL_BUTTON_RIGHT: return HELL_MOUSE_RIGHT;
            // add middle if you want:
            // case SDL_BUTTON_MIDDLE: return HELL_MOUSE_MIDDLE;
            default: return -1;
        }
    }
}
