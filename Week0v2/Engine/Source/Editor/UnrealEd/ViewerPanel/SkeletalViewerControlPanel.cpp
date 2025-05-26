#include "SkeletalViewerControlPanel.h"

#include "Components/Mesh/SkeletalMesh.h"
#include "ImGui/imgui.h"
#include "UObject/UObjectIterator.h"

void FSkeletalViewerControlPanel::Render()
{
    /* Pre Setup */
    // Menu bar
    const ImGuiIO& IO = ImGui::GetIO();
    ImFont* IconFont = IO.Fonts->Fonts.size() == 1 ? IO.FontDefault : IO.Fonts->Fonts[FEATHER_FONT];
    constexpr ImVec2 IconSize = ImVec2(32, 32);
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2(0, 20));

    ImGui::SetNextWindowSize(ImVec2(Width, 50));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255)); 
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags =ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    /* Render Start */
    if (ImGui::Begin("Control Panel", nullptr, PanelFlags))
    {
        ImGui::PushFont(IconFont);
        
        if (ImGui::Button("\ue9d6",IconSize))
        {
            FArchive Ar;
            SkeletalMesh->Serialize(Ar);
            Ar.SaveToFile("aaaa.bin");
        }
        ImGui::SameLine();
        if (ImGui::Button("\ue950",IconSize))
        {
            FArchive Ar;
            USkeletalMesh* SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
            Ar.LoadFromFile("aaaa.bin");
            SkeletalMesh->GetRenderData().Name = "Aaaa";
            SkeletalMesh->Deserialize(Ar);
        }
        ImGui::PopFont();
        
        ImGui::End();
    }
    ImGui::PopStyleColor();
}

void FSkeletalViewerControlPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
