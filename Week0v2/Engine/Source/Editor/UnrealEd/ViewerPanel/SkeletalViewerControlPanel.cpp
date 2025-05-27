#include "SkeletalViewerControlPanel.h"

#include "Animation/Skeleton.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "ImGui/imgui.h"
#include "UObject/UObjectIterator.h"
#include "tinyfiledialogs/tinyfiledialogs.h"

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
            char const* lFilterPatterns[1] = { "*.DDAL" };
            const char* FileName = tinyfd_saveFileDialog("Save Skeletal File", "Contents/MySkeletal", 1, lFilterPatterns, "DDAL(.DDAL) file");

            if (FileName == nullptr)
            {
                ImGui::PopFont();
                ImGui::End();
                ImGui::PopStyleColor();
                return;
            }
            FArchive Ar;
            SkeletalMesh->Serialize(Ar);
            Ar.SaveToFile(FileName);
        }
        ImGui::SameLine();
        if (ImGui::Button("\ue950",IconSize))
        {
            char const* lFilterPatterns[1] = { "*.DDAL" };
            const char* FileName = tinyfd_openFileDialog("Open Skeletal File", "Contents/MySkeletal", 1, lFilterPatterns, "DDAL(.DDAL) file", 0);

            if (FileName == nullptr)
            {
                tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                ImGui::PopFont();
                ImGui::End();
                ImGui::PopStyleColor();
                return;
            }

            FArchive Ar;
            USkeletalMesh* SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
            Ar.LoadFromFile(FileName);
            SkeletalMesh->Deserialize(Ar);
            SkeletalMesh->GetRenderData().Name = FileName;
            SkeletalMesh->GetSkeleton()->GetRefSkeletal()->Name = FileName;
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
