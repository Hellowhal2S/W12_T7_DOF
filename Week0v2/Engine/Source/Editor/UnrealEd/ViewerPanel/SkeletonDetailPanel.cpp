#include "SkeletonDetailPanel.h"
#include "ImGui/imgui.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/ConstraintSetup.h"

void FSkeletonDetailPanel::Render(UBodySetup* BodySetup, TArray<UConstraintSetup*>& ConstraintSetups)
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

        // --- Constraint Elements ---
        if (ImGui::TreeNodeEx("Constraints", ImGuiTreeNodeFlags_Framed))
        {
            if (ConstraintSetups.Num() > 0)
            {
                for (UConstraintSetup* Constraint : ConstraintSetups)
                {
                    // BodySetup->BoneName과 연결된 Constraint만 출력/수정
                    if (Constraint &&
                        (Constraint->JointElem.ChildBoneName == BodySetup->BoneName ||
                         Constraint->JointElem.ParentBoneName == BodySetup->BoneName))
                    {
                        ImGui::Text("Constraint: %s  %s <-> %s",
                            *Constraint->JointName.ToString(),
                            *Constraint->JointElem.ParentBoneName.ToString(),
                            *Constraint->JointElem.ChildBoneName.ToString());

                        // Twist 제한(각도, degree 단위로 표시/입력)
                        float twistMin = FMath::RadiansToDegrees(Constraint->JointElem.TwistLimitMin);
                        float twistMax = FMath::RadiansToDegrees(Constraint->JointElem.TwistLimitMax);
                        if (ImGui::InputFloat("Twist Min (deg)", &twistMin))
                            Constraint->JointElem.TwistLimitMin = FMath::DegreesToRadians(twistMin);
                        if (ImGui::InputFloat("Twist Max (deg)", &twistMax))
                            Constraint->JointElem.TwistLimitMax = FMath::DegreesToRadians(twistMax);

                        // Swing 제한
                        float swingMin1 = FMath::RadiansToDegrees(Constraint->JointElem.SwingLimitMin1);
                        float swingMax1 = FMath::RadiansToDegrees(Constraint->JointElem.SwingLimitMax1);
                        if (ImGui::InputFloat("Swing Min1 (deg)", &swingMin1))
                            Constraint->JointElem.SwingLimitMin1 = FMath::DegreesToRadians(swingMin1);
                        if (ImGui::InputFloat("Swing Max1 (deg)", &swingMax1))
                            Constraint->JointElem.SwingLimitMax1 = FMath::DegreesToRadians(swingMax1);

                        // Swing 제한
                        float swingMin2 = FMath::RadiansToDegrees(Constraint->JointElem.SwingLimitMin2);
                        float swingMax2 = FMath::RadiansToDegrees(Constraint->JointElem.SwingLimitMax2);
                        if (ImGui::InputFloat("Swing Min2 (deg)", &swingMin2))
                            Constraint->JointElem.SwingLimitMin2 = FMath::DegreesToRadians(swingMin2);
                        if (ImGui::InputFloat("Swing Max2 (deg)", &swingMax2))
                            Constraint->JointElem.SwingLimitMax2 = FMath::DegreesToRadians(swingMax2);

                        // 축별 제한(ComboBox)
                        static const char* axisNames[] = { "X", "Y", "Z", "Twist", "Swing1", "Swing2" };
                        static const char* motionNames[] = { "Locked", "Limited", "Free" };
                        for (int axis = 0; axis < 6; ++axis)
                        {
                            int motion = static_cast<int>(Constraint->JointElem.AxisMotions[axis]);
                            if (ImGui::Combo(axisNames[axis], &motion, motionNames, 3))
                                Constraint->JointElem.AxisMotions[axis] = static_cast<EJointMotion>(motion);
                        }
                    }
                }
            }
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
