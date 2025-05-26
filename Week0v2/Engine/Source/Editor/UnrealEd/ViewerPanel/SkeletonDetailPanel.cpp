#include "SkeletonDetailPanel.h"
#include "ImGui/imgui.h"
#include "PhysicsEngine/BodySetup.h"

void FSkeletonDetailPanel::Render(UBodySetup* BodySetup)
{

    ImVec2 WinSize = ImVec2(Width, Height);

    ImGui::SetNextWindowPos(ImVec2( (Width) * 0.8f + 5.0f, 70.0f));

    ImGui::SetNextWindowSize(ImVec2((Width) * 0.2f - 6.0f,  (Height) -70));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;

    if (!ImGui::Begin("Details", nullptr, PanelFlags))
    {
        
       ImGui::End();
    }

    if (BodySetup==nullptr)
    {
        ImGui::End();
        return;
    }
    
    FKAggregateGeom& Agg = BodySetup->AggGeom;

    // -- Primitives Header --
    if (ImGui::CollapsingHeader("Primitives", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // --- Spheres ---
        if (ImGui::TreeNodeEx("Spheres", ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Text("Count: %d", Agg.SphereElems.Num());
            for (int i = 0; i < Agg.SphereElems.Num(); ++i)
            {
                FKSphereElem& S = Agg.SphereElems[i];
                char buf[64];
                sprintf_s(buf, "Sphere %d", i);
                if (ImGui::TreeNode(buf))
                {
                    // Center
                    ImGui::InputFloat3("Center", &S.Center.X, "%.3f");
                    // Radius
                    ImGui::InputFloat("Radius", &S.Radius, 1.0f, 10.0f, "%.3f");
                    ImGui::Checkbox("Collision Generated", &S.bEnableCollision);
                    ImGui::TreePop();
                }
            }
            if (ImGui::Button("Add Sphere"))  { Agg.SphereElems.Add(FKSphereElem()); }
            ImGui::TreePop();
        }

        // --- Boxes ---
        if (ImGui::TreeNodeEx("Boxes", ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Text("Count: %d", Agg.BoxElems.Num());
            for (int i = 0; i < Agg.BoxElems.Num(); ++i)
            {
                FKBoxElem& B = Agg.BoxElems[i];
                char buf[64];
                sprintf_s(buf, "Box %d", i);
                if (ImGui::TreeNode(buf))
                {
                    // Center
                    ImGui::InputFloat3("Center", &B.Center.X, "%.3f");
                    // Extents
                    ImGui::InputFloat3("Extents", &B.X, "%.3f");
                    ImGui::Checkbox("Collision Generated", &B.bEnableCollision);
                    ImGui::TreePop();
                }
            }
            if (ImGui::Button("Add Box")) { Agg.BoxElems.Add(FKBoxElem()); }
            ImGui::TreePop();
        }

        // --- Capsules ---
        if (ImGui::TreeNodeEx("Capsules", ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Text("Count: %d", Agg.SphylElems.Num());
            for (int i = 0; i < Agg.SphylElems.Num(); ++i)
            {
                FKSphylElem& C = Agg.SphylElems[i];
                char buf[64];
                sprintf_s(buf, "Capsule %d", i);
                if (ImGui::TreeNode(buf))
                {
                    ImGui::InputFloat3("Center", &C.Center.X, "%.3f");
                    ImGui::InputFloat3("Rotation", &C.Rotation.Pitch, "%.3f"); // assume rotation stored as FRotator
                    ImGui::InputFloat("Radius", &C.Radius, 1.0f, 10.0f, "%.3f");
                    ImGui::InputFloat("Length", &C.Length, 1.0f, 10.0f, "%.3f");
                    ImGui::Checkbox("Collision Generated", &C.bEnableCollision);
                    ImGui::TreePop();
                }
            }
            if (ImGui::Button("Add Capsule")) { Agg.SphylElems.Add(FKSphylElem()); }
            ImGui::TreePop();
        }

        // --- Convex Elements ---
        if (ImGui::TreeNodeEx("Convex Elements", ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Text("Count: %d", Agg.ConvexElems.Num());
            for (int i = 0; i < Agg.ConvexElems.Num(); ++i)
            {
                FKConvexElem& X = Agg.ConvexElems[i];
                char buf[64];
                sprintf_s(buf, "Convex %d", i);
                if (ImGui::TreeNode(buf))
                {
                    ImGui::Text("Vertices: %d", X.VertexData.Num());
                    ImGui::Checkbox("Collision Generated", &X.bEnableCollision);
                    ImGui::TreePop();
                }
            }
            if (ImGui::Button("Add Convex")) { Agg.ConvexElems.Add(FKConvexElem()); }
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void FSkeletonDetailPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
