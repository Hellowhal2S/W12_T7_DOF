#include "SkeletonDetailPanel.h"

#include "ImGui/imgui.h"

void FSkeletonDetailPanel::Render()
{
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2( (Width) * 0.8f + 5.0f, 70.0f));

    ImGui::SetNextWindowSize(ImVec2((Width) * 0.2f - 6.0f,  (Height) -70));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;

    if (ImGui::Begin("Details", nullptr, PanelFlags))
    {
       ImGui::End();
    }
}

void FSkeletonDetailPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
