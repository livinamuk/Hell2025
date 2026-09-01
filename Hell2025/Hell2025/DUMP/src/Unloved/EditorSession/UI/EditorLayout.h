#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

namespace Unloved::EditorSession::Layout {

    void Update();
    void UpdateDividerInput(bool allowInput);
    void RenderBackgrounds();
    void RenderOverlay();
    void CancelInteraction();

    void SetFileMenuHeight(int32_t height);
    void SetHierarchyWidth(int32_t width);
    void SetPropertiesWidth(int32_t width);
    void SetPropertiesContentHeight(int32_t height);
    void SetToolsContentHeight(int32_t height);
    void SetToolsVisible(bool visible);
    void SetBrushesVisible(bool visible);
    void SetMaterialsVisible(bool visible);
    void SetViewportLayout(EditorViewportLayout layout);
    void SetPanelEdges(EditorPanelId panelId, EditorPanelEdge edges);

    int32_t GetFileMenuHeight();
    int32_t GetHierarchyWidth();
    int32_t GetPropertiesWidth();
    uint32_t GetViewportCount();
    EditorViewportLayout GetViewportLayout();
    bool WantsMouseCapture();

    const EditorPanel& GetFileMenuPanel();
    const EditorPanel& GetHierarchyPanel();
    const EditorPanel& GetViewportsPanel();
    const EditorPanel& GetPropertiesPanel();
    const EditorPanel& GetToolsPanel();
    const EditorPanel& GetBrushesPanel();
    const EditorPanel& GetMaterialsPanel();
    EditorRect GetHierarchyContentRect();
    EditorRect GetPropertiesContentRect();
    EditorRect GetToolsContentRect();
    EditorRect GetBrushesContentRect();
    EditorRect GetMaterialsContentRect();
    const EditorViewportRegion* GetViewportRegionByIndex(uint32_t index);
}
