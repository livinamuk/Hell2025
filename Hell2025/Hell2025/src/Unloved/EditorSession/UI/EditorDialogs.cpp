#include "EditorDialogs.h"

#include "EditorInputElements.h"
#include "EditorStyle.h"
#include "EditorUI.h"
#include "Unloved/EditorSession/EditorSession.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/File/File.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"

#include <algorithm>
#include <utility>
#include <vector>

// Shared dialog layout
namespace Unloved::EditorSession {
    namespace {
        EditorRect GetCenteredWindowRect(const EditorRect& canvasRect, int32_t requestedWidth, int32_t requestedHeight) {
            const int32_t windowPadding = GetStyle().modal.windowPadding;
            const int32_t windowWidth = std::min(requestedWidth, std::max(0, canvasRect.width - windowPadding * 2));
            const int32_t windowHeight = std::min(requestedHeight, std::max(0, canvasRect.height - windowPadding * 2));
            return { (canvasRect.width - windowWidth) / 2, (canvasRect.height - windowHeight) / 2, windowWidth, windowHeight };
        }

        void DrawDialogWindow(const EditorRect& canvasRect, const EditorRect& windowRect, const char* title) {
            const EditorStyle& style = GetStyle();
            const int32_t titleBarHeight = std::min(style.modal.titleBarHeight, windowRect.height);

            // Outer border stays with each dialog for draw ordering
            UI::DrawSolidRect(canvasRect, style.colors.overlay);
            UI::DrawSolidRect(windowRect, style.colors.panelBackground);
            UI::DrawSolidRect({ windowRect.x, windowRect.y, windowRect.width, titleBarHeight }, style.colors.controlBackground);
            UI::DrawSolidRect({ windowRect.x, windowRect.y + titleBarHeight, windowRect.width, 1 }, style.colors.border);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + title, style.font.name, glm::ivec2(windowRect.x + style.modal.windowPadding, windowRect.y + style.modal.titleBarHeight / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST, windowRect.x, windowRect.y, windowRect.Right(), windowRect.y + style.modal.titleBarHeight);
        }
    }
}

// Warning dialog
namespace Unloved::EditorSession::Dialog {
    namespace {
        std::string g_message;
        bool g_isOpen = false;
    }

    void Open(const std::string& message) {
        g_message = message;
        g_isOpen = true;
    }

    void Close() {
        g_message.clear();
        g_isOpen = false;
    }

    void Render() {
        if (!g_isOpen) return;

        const EditorStyle& style = GetStyle();
        const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
        const EditorRect canvasRect = { 0, 0, static_cast<int32_t>(resolution.x), static_cast<int32_t>(resolution.y) };
        const EditorRect windowRect = GetCenteredWindowRect(canvasRect, style.dialog.windowWidth, style.dialog.windowHeight);
        const EditorRect buttonRect = { windowRect.x + (windowRect.width - style.modal.buttonWidth) / 2, windowRect.Bottom() - style.modal.windowPadding - style.modal.buttonHeight, style.modal.buttonWidth, style.modal.buttonHeight };
        const EditorRect messageRect = { windowRect.x + style.modal.windowPadding, windowRect.y + style.modal.titleBarHeight + 1, windowRect.width - style.modal.windowPadding * 2, buttonRect.y - windowRect.y - style.modal.titleBarHeight - 1 };
        const bool buttonHovered = buttonRect.Contains(Coordinates::GetMousePositionUI());
        const bool dismiss = buttonHovered && Hell::Input::LeftMousePressed();

        // Dialog window
        DrawDialogWindow(canvasRect, windowRect, "Warning");
        UI::DrawBorder(windowRect, style.colors.border);

        // Dialog message
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + g_message, style.font.name, glm::ivec2(messageRect.x + messageRect.width / 2, messageRect.y + messageRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, messageRect.x, messageRect.y, messageRect.Right(), messageRect.Bottom());

        // Dismiss button
        UI::DrawSolidRect(buttonRect, buttonHovered ? style.colors.hover : style.colors.controlBackground);
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + "Dismiss", style.font.name, glm::ivec2(buttonRect.x + buttonRect.width / 2, buttonRect.y + buttonRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, buttonRect.x, buttonRect.y, buttonRect.Right(), buttonRect.Bottom());

        // Interaction
        if (buttonHovered) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }

        if (dismiss || Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) {
            Close();
        }
    }

    bool IsOpen() {
        return g_isOpen;
    }
}

// File dialogs
namespace Unloved::EditorSession::FileDialog {
    namespace {
        constexpr int32_t ROWS_PER_SCROLL = 3;
        constexpr uint64_t NEW_FILE_INPUT_ID = UINT64_MAX - 2;

        struct FileDialogEntry {
            std::string label;
            std::string value;
        };

        std::vector<FileDialogEntry> g_files;
        std::string g_selectedFile;
        std::string g_pendingFile;
        std::string g_pendingImportedRagdoll;
        std::string g_newFileName;
        EditorScrollBar g_scrollBar;
        EditorSessionMode g_mode = EditorSessionMode::HOUSE;
        NewFileDialogType g_newFileDialogType = NewFileDialogType::NONE;
        bool g_isImportingRagdoll = false;
        bool g_isOpen = false;

        void DrawButton(const EditorRect& rect, const char* text, bool enabled, bool hovered) {
            const EditorStyle& style = GetStyle();
            UI::DrawSolidRect(rect, enabled && hovered ? style.colors.hover : style.colors.controlBackground);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + text, style.font.name, glm::ivec2(rect.x + rect.width / 2, rect.y + rect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, rect.x, rect.y, rect.Right(), rect.Bottom());
        }

        void Hide() {
            g_isOpen = false;
            g_newFileDialogType = NewFileDialogType::NONE;
            g_isImportingRagdoll = false;
            g_files.clear();
            g_selectedFile.clear();
            g_newFileName.clear();
            g_scrollBar = {};
        }

        void AddFiles(const char* directory, const char* extension, const char* labelPrefix, bool returnFullPath) {
            const bool hasLabelPrefix = labelPrefix[0] != '\0';

            for (const FileInfo& fileInfo : Hell::File::IterateDirectory(directory, { extension })) {
                FileDialogEntry& file = g_files.emplace_back();
                file.label = hasLabelPrefix ? std::string(labelPrefix) + "/" + fileInfo.name : fileInfo.name;
                file.value = returnFullPath ? fileInfo.path : fileInfo.name;
            }
        }

        void SubmitFileName() {
            const NewFileDialogType type = g_newFileDialogType;
            const std::string fileName = g_newFileName;
            Hide();
            if (type == NewFileDialogType::SAVE_RAGDOLL_AS) EditorSession::SaveRagdollAs(fileName);
            else EditorSession::CreateNewFile(type, fileName);
        }

        void RenderNewFileDialog(const EditorRect& canvasRect) {
            const EditorStyle& style = GetStyle();

            // Calculate the dialog layout
            const EditorRect windowRect = GetCenteredWindowRect(canvasRect, style.fileDialog.newWindowWidth, style.fileDialog.newWindowHeight);
            const int32_t buttonRowWidth = style.modal.buttonWidth * 2 + style.fileDialog.buttonGap;
            const EditorRect cancelButtonRect = { windowRect.x + (windowRect.width - buttonRowWidth) / 2, windowRect.Bottom() - style.modal.windowPadding - style.modal.buttonHeight, style.modal.buttonWidth, style.modal.buttonHeight };
            const EditorRect createButtonRect = { cancelButtonRect.Right() + style.fileDialog.buttonGap, cancelButtonRect.y, style.modal.buttonWidth, style.modal.buttonHeight };
            const int32_t inputAreaY = windowRect.y + style.modal.titleBarHeight + 1;
            const int32_t inputAreaHeight = cancelButtonRect.y - inputAreaY;
            const int32_t inputHeight = style.input.contentPadding * 2 + style.input.rowHeight;
            const EditorRect inputRect = { windowRect.x + style.modal.windowPadding, inputAreaY + (inputAreaHeight - inputHeight) / 2, windowRect.width - style.modal.windowPadding * 2, inputHeight };
            const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();

            // Dialog window
            const char* title = "New File";
            switch (g_newFileDialogType) {
                case NewFileDialogType::NEW_HOUSE:       title = "New House"; break;
                case NewFileDialogType::NEW_MAP:         title = "New Map"; break;
                case NewFileDialogType::NEW_RAGDOLL:     title = "New Ragdoll"; break;
                case NewFileDialogType::NEW_BONE_MASK:   title = "New Bone Mask"; break;
                case NewFileDialogType::SAVE_RAGDOLL_AS: title = "Save Ragdoll As"; break;
                case NewFileDialogType::NONE:            return;
            }
            DrawDialogWindow(canvasRect, windowRect, title);
            UI::DrawBorder(windowRect, style.colors.border);

            // File name input
            bool submit = false;
            InputElements::PropertyList properties;
            properties.String(NEW_FILE_INPUT_ID, "Name", g_newFileName, [&] { submit = Hell::Input::KeyPressed(HELL_KEY_ENTER); }, true);
            properties.Render(inputRect);

            // Dialog buttons
            const bool allowInput = !Dialog::IsOpen();
            const bool propertyConsumedMousePress = InputElements::DidConsumeMousePress();
            const bool cancelHovered = allowInput && cancelButtonRect.Contains(mousePosition);
            const bool createEnabled = !g_newFileName.empty();
            const bool createHovered = allowInput && createEnabled && createButtonRect.Contains(mousePosition);
            const bool dialogButtonHovered = cancelHovered || createHovered;
            DrawButton(cancelButtonRect, "Cancel", true, cancelHovered);
            DrawButton(createButtonRect, g_newFileDialogType == NewFileDialogType::SAVE_RAGDOLL_AS ? "Save" : "Create", createEnabled, createHovered);

            // Interaction
            if (!allowInput) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            }

            if (dialogButtonHovered) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            }

            const bool cancelPressed = !propertyConsumedMousePress && cancelHovered && Hell::Input::LeftMousePressed();
            const bool closeRequested = allowInput && (cancelPressed || Hell::Input::KeyPressed(HELL_KEY_ESCAPE));
            if (closeRequested) {
                Hide();
            }
            else {
                const bool createPressed = !propertyConsumedMousePress && createHovered && Hell::Input::LeftMousePressed();
                if (!g_newFileName.empty() && (submit || createPressed)) {
                    SubmitFileName();
                }
            }
        }
    }

    void Open(EditorSessionMode mode, const std::string& selectedFileName) {
        Close();
        InputElements::Reset();
        g_mode = mode;
        switch (mode) {
            case EditorSessionMode::HOUSE:
                AddFiles("res/houses", "house", "", false);
                break;
            case EditorSessionMode::MAP:
                AddFiles("res/maps", "map", "", false);
                break;
            case EditorSessionMode::RAGDOLL:
                AddFiles("res/ragdolls", "ragdoll", "", true);
                break;
            case EditorSessionMode::BONE_MASK:
                AddFiles("res/bone_masks", "bonemask", "", true);
                break;
        }

        std::sort(g_files.begin(), g_files.end(), [](const FileDialogEntry& a, const FileDialogEntry& b) { return a.label < b.label; });

        const auto selectedFile = std::find_if(g_files.begin(), g_files.end(), [&](const FileDialogEntry& file) { return file.value == selectedFileName; });
        if (selectedFile != g_files.end()) {
            g_selectedFile = selectedFileName;
        }

        g_isOpen = true;
    }

    void ImportRagdoll(const std::string& selectedFileName) {
        Close();
        InputElements::Reset();
        g_mode = EditorSessionMode::RAGDOLL;
        g_isImportingRagdoll = true;
        AddFiles("res/ragdolls/dynamics", "rag", "", true);
        std::sort(g_files.begin(), g_files.end(), [](const FileDialogEntry& a, const FileDialogEntry& b) { return a.label < b.label; });

        const auto selectedFile = std::find_if(g_files.begin(), g_files.end(), [&](const FileDialogEntry& file) { return file.value == selectedFileName; });
        if (selectedFile != g_files.end()) g_selectedFile = selectedFileName;
        g_isOpen = true;
    }

    void New(NewFileDialogType type) {
        Close();
        g_newFileDialogType = type;
        g_isOpen = true;
        InputElements::FocusString(NEW_FILE_INPUT_ID, "Name", g_newFileName);
    }

    void SaveRagdollAs(const std::string& initialName) {
        Close();
        g_newFileName = initialName;
        g_newFileDialogType = NewFileDialogType::SAVE_RAGDOLL_AS;
        g_isOpen = true;
        InputElements::FocusString(NEW_FILE_INPUT_ID, "Name", g_newFileName);
    }

    void Close() {
        Hide();
        g_pendingFile.clear();
        g_pendingImportedRagdoll.clear();
    }

    void Render() {
        if (!g_isOpen) return;

        const EditorStyle& style = GetStyle();
        const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
        const EditorRect canvasRect = { 0, 0, static_cast<int32_t>(resolution.x), static_cast<int32_t>(resolution.y) };
        if (g_newFileDialogType != NewFileDialogType::NONE) {
            RenderNewFileDialog(canvasRect);
            return;
        }

        // Calculate the dialog layout
        const EditorRect windowRect = GetCenteredWindowRect(canvasRect, style.fileDialog.windowWidth, style.fileDialog.windowHeight);
        const int32_t buttonRowWidth = style.modal.buttonWidth * 2 + style.fileDialog.buttonGap;

        const EditorRect openButtonRect = { windowRect.x + (windowRect.width - buttonRowWidth) / 2, windowRect.Bottom() - style.modal.windowPadding - style.modal.buttonHeight, style.modal.buttonWidth, style.modal.buttonHeight };
        const EditorRect cancelButtonRect = { windowRect.x + (windowRect.width - buttonRowWidth) / 2 + style.modal.buttonWidth + style.fileDialog.buttonGap, windowRect.Bottom() - style.modal.windowPadding - style.modal.buttonHeight, style.modal.buttonWidth, style.modal.buttonHeight };
        const EditorRect listRect = { windowRect.x + style.modal.windowPadding, windowRect.y + style.modal.titleBarHeight + style.modal.windowPadding, windowRect.width - style.modal.windowPadding * 2, cancelButtonRect.y - style.fileDialog.buttonGap - windowRect.y - style.modal.titleBarHeight - style.modal.windowPadding };

        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const int32_t visibleRowCount = std::max(0, listRect.height / style.fileDialog.rowHeight);
        const char* dialogTitle = g_isImportingRagdoll ? "Import .rag" : "Open File";
        const char* openButtonText = g_isImportingRagdoll ? "Import" : "Open";

        // Scroll the file list
        if (listRect.Contains(mousePosition)) {
            if (Hell::Input::MouseWheelUp()) {
                g_scrollBar.value -= ROWS_PER_SCROLL;
            }
            else if (Hell::Input::MouseWheelDown()) {
                g_scrollBar.value += ROWS_PER_SCROLL;
            }
        }

        ScrollBar::Update(g_scrollBar, { listRect.Right() - style.fileDialog.scrollBarWidth, listRect.y, style.fileDialog.scrollBarWidth, listRect.height }, static_cast<int32_t>(g_files.size()), visibleRowCount, true);
        const int32_t rowWidth = listRect.width - (g_scrollBar.visible ? style.fileDialog.scrollBarWidth : 0);

        // Dialog window
        DrawDialogWindow(canvasRect, windowRect, dialogTitle);
        UI::DrawSolidRect(listRect, style.colors.controlBackground);

        // File rows
        for (int32_t visibleIndex = 0; visibleIndex < visibleRowCount; visibleIndex++) {
            const int32_t fileIndex = g_scrollBar.value + visibleIndex;
            if (fileIndex < 0 || fileIndex >= static_cast<int32_t>(g_files.size())) {
                break;
            }

            const EditorRect rowRect = { listRect.x, listRect.y + visibleIndex * style.fileDialog.rowHeight, rowWidth, style.fileDialog.rowHeight };
            const FileDialogEntry& file = g_files[fileIndex];
            const bool hovered = rowRect.Contains(mousePosition);

            if (file.value == g_selectedFile) {
                UI::DrawSolidRect(rowRect, style.colors.selected);
            }
            else if (hovered) {
                UI::DrawSolidRect(rowRect, style.colors.hover);
            }

            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + file.label, style.font.name, glm::ivec2(rowRect.x + style.modal.windowPadding, rowRect.y + rowRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST, rowRect.x, rowRect.y, rowRect.Right(), rowRect.Bottom());

            const bool filePressed = hovered && Hell::Input::LeftMousePressed();
            if (filePressed) {
                g_selectedFile = file.value;
            }
        }

        // Dialog buttons
        const bool cancelHovered = cancelButtonRect.Contains(mousePosition);
        const bool openEnabled = !g_selectedFile.empty();
        const bool openHovered = openEnabled && openButtonRect.Contains(mousePosition);

        DrawButton(openButtonRect, openButtonText, openEnabled, openHovered);
        DrawButton(cancelButtonRect, "Cancel", true, cancelHovered);

        ScrollBar::Render(g_scrollBar);
        UI::DrawBorder(windowRect, style.colors.border);

        // Interaction
        const bool mouseOverInteractiveArea = cancelHovered || openHovered || listRect.Contains(mousePosition);
        if (mouseOverInteractiveArea) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }

        const bool cancelPressed = cancelHovered && Hell::Input::LeftMousePressed();
        const bool closeRequested = cancelPressed || Hell::Input::KeyPressed(HELL_KEY_ESCAPE);
        if (closeRequested) {
            Hide();
        }
        else {
            const bool openPressed = openHovered && Hell::Input::LeftMousePressed();
            if (openPressed) {
                if (g_isImportingRagdoll) g_pendingImportedRagdoll = g_selectedFile;
                else g_pendingFile = g_selectedFile;
                Hide();
            }
        }
    }

    bool IsOpen() {
        return g_isOpen;
    }

    bool IsNameInputOpen() {
        return g_isOpen && g_newFileDialogType != NewFileDialogType::NONE;
    }

    std::string ConsumeSelectedFile() {
        std::string file = std::move(g_pendingFile);
        g_pendingFile.clear();
        return file;
    }

    std::string ConsumeImportedRagdoll() {
        std::string file = std::move(g_pendingImportedRagdoll);
        g_pendingImportedRagdoll.clear();
        return file;
    }

}
