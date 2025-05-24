#include "PhysicsToolBoxPanel.h"

#include "ImGui/imgui.h"

void FPhysicsToolBoxPanel::Render()
{
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2(0, 70));

    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.3f, Height -70));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255)); 
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;

    if (ImGui::Begin("Toolbox", nullptr, PanelFlags))
    {

    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void FPhysicsToolBoxPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
