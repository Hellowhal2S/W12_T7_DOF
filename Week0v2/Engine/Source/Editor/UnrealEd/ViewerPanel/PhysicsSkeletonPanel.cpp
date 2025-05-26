#include "PhysicsSkeletonPanel.h"

#include "ImGUI/imgui.h"


#include "Engine/World.h"
#include "Engine/FLoaderOBJ.h"
#include "UnrealEd/ImGuiWidget.h"


#include "Components/GameFramework/RotatingMovementComponent.h"
#include "Components/LuaComponent.h"

#include "Components/Mesh/StaticMesh.h"

#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/CubeComp.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"

#include "LevelEditor/SLevelEditor.h"

#include "LaunchEngineLoop.h"
#include "PlayerCameraManager.h"
#include "Engine/FBXLoader.h"
#include "Animation/Skeleton.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Light/ShadowMapAtlas.h"
#include "PhysicsEngine/BodySetup.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "UObject/FunctionRegistry.h"

void FPhysicsSkeletonPanel::Initialize(float InWidth, float InHeight)
{
    Width = InWidth;
    Height = InHeight;
}

void FPhysicsSkeletonPanel::Render()
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (EditorEngine == nullptr)
    {
        return;
    }
    ImVec2 WinSize = ImVec2(Width, Height);
    

    ImVec2 MinSize(140, 370);
    ImVec2 MaxSize(FLT_MAX, 900);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    ImGui::SetNextWindowPos(ImVec2(0, 70));

    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.3f, Height -70));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255)); 

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    /* Render Start */
    ImGui::Begin("Skeleton Tree", nullptr, PanelFlags);

    // 프리뷰 월드에서는 오로지 하나의 액터만 선택 가능 (스켈레탈 메쉬 프리뷰인 경우, 스켈레탈 메쉬 액터)
    AActor* PickedActor = nullptr;

    if (!World->GetSelectedActors().IsEmpty())
    {
        PickedActor = *World->GetSelectedActors().begin();
    }

    ImVec2 imageSize = ImVec2(256, 256); // 이미지 출력 크기
    

    if (PickedActor)
    {
        if (PickedComponent && PickedComponent->GetOwner() && PickedComponent->GetOwner() != PickedActor)
        {
            // 다른 액터를 픽한 것 -> PickedComponent를 PickedActor의 RootComponent로 바꿔준다
            PickedComponent = PickedActor->GetRootComponent();
        }
    }

    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor)
    {
        if (UStaticMeshComponent* StaticMeshComponent = PickedActor->GetComponentByClass<UStaticMeshComponent>())
        {
        }
        else if (USkeletalMeshComponent* SkeletalMeshComponet = PickedActor->GetComponentByClass<USkeletalMeshComponent>())
        {
            RenderForSkeletalMesh(SkeletalMeshComponet);
            SkeletalMesh = SkeletalMeshComponet->GetSkeletalMesh();
            PhysicsAsset = SkeletalMeshComponet->GetSkeletalMesh()->GetPhysicsAsset();
        }
    }

    RenderShapeProperty(PickedActor);
    ImGui::End();

    UPrimitiveBatch& PrimitiveBatch = UPrimitiveBatch::GetInstance();
    
    PhysicsDetailPanel.Render(SelectedBodySetup);

    ImGui::PopStyleColor();
}


void FPhysicsSkeletonPanel::RenderForSkeletalMesh(USkeletalMeshComponent* SkeletalMesh)
{
    if (SkeletalMesh->GetSkeletalMesh() == nullptr)
    {
        return;
    }
    
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Skeletal Mesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Skeletal Mesh");
        ImGui::SameLine();

        // 본 제어 섹션
        if (ImGui::CollapsingHeader("Bone Controls", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 회전값 변경 여부를 추적
            bool anyRotationChanged = false;

            // 선택된 본이 있을 경우 회전 컨트롤 표시
            if (SelectedBoneIndex!=-1)
            {
                ImGui::Separator();
                ImGui::Text("Selected Bone: %s", GetData(SkeletalMesh->GetSkeletalMesh()->GetRenderData().Bones[SelectedBoneIndex].BoneName));

                FBoneRotation* foundRotation = BoneRotations.Find(SelectedBoneIndex);
                if (foundRotation)
                {
                    // 저장된 회전값이 있으면 사용
                    XRotation = foundRotation->X;
                    YRotation = foundRotation->Y;
                    ZRotation = foundRotation->Z;
                }
                else
                {
                     XRotation = 0.f;
                     YRotation = 0.f;
                     ZRotation = 0.f;
                }

                // 회전 슬라이더
                bool rotationChanged = false;
                rotationChanged |= ImGui::SliderFloat("X Rotation", &XRotation, -180.0f, 180.0f, "%.1f°");
                rotationChanged |= ImGui::SliderFloat("Y Rotation", &YRotation, -180.0f, 180.0f, "%.1f°");
                rotationChanged |= ImGui::SliderFloat("Z Rotation", &ZRotation, -180.0f, 180.0f, "%.1f°");

                // 슬라이더 값이 변경되면 맵에 저장
                if (rotationChanged)
                {
                    if (BoneRotations.Contains(SelectedBoneIndex))
                    {
                        PrevBoneRotations[SelectedBoneIndex] = BoneRotations[SelectedBoneIndex];
                    }
                    else
                    {
                        PrevBoneRotations[SelectedBoneIndex] = FBoneRotation(0, 0, 0);
                    }
                    BoneRotations[SelectedBoneIndex] = FBoneRotation(XRotation, YRotation, ZRotation);
                    anyRotationChanged = true;
                }
            }

            // 회전값이 변경된 경우에만 적용
            if (anyRotationChanged)
            {
                // 저장된 모든 본 회전을 적용
                for (uint32 i = 0; i < BoneRotations.Num(); i++)
                {
                    const int& BoneName = i;
                    const FBoneRotation Rotation = BoneRotations[i];
                    const FBoneRotation PrevRotation = PrevBoneRotations[i];

                    PrevBoneRotations[i] = BoneRotations[i];
                    float X, Y, Z;
                    X = Rotation.X - PrevRotation.X;
                    Y = Rotation.Y - PrevRotation.Y;
                    Z = Rotation.Z - PrevRotation.Z;

                    int boneIndex = BoneName;
                    if (boneIndex >= 0)
                    {
                        // 각 축별로 회전 적용 (로컬 변환만)
                        SkeletalMesh->GetSkeletalMesh()->RotateBoneByIndex(boneIndex, X, FVector::ForwardVector, false);
                        SkeletalMesh->GetSkeletalMesh()->RotateBoneByIndex(boneIndex, Y, FVector::RightVector, false);
                        SkeletalMesh->GetSkeletalMesh()->RotateBoneByIndex(boneIndex, Z, FVector::UpVector, false);
                    }
                }

                // 모든 회전 적용 후 한 번만 업데이트
                SkeletalMesh->GetSkeletalMesh()->UpdateBoneHierarchy();
                SkeletalMesh->GetSkeletalMesh()->UpdateSkinnedVertices();
            }
        }

        // 계층적 본 구조 표시
        if (ImGui::CollapsingHeader("Bone Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // RefSkeletal이 없는 경우 처리
            if (SkeletalMesh->GetSkeletalMesh()->GetSkeleton() == nullptr)
            {
                ImGui::Text("No skeletal hierarchy available");
            }
            else
            {
                // 루트 본부터 계층 구조 표시
                for (const auto& RootBoneIndex : SkeletalMesh->GetSkeletalMesh()->GetSkeleton()->GetRefSkeletal()->RootBoneIndices)
                {
                    RenderBoneHierarchy(SkeletalMesh->GetSkeletalMesh(), RootBoneIndex);
                }
            }
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void FPhysicsSkeletonPanel::RenderBoneHierarchy(USkeletalMesh* SkeletalMesh, int32 BoneIndex)
{
    // 범위 체크
    if (BoneIndex < 0 || BoneIndex >= SkeletalMesh->GetRenderData().Bones.Num())
        return;

    // 본 이름 가져오기
    const FString& boneName = SkeletalMesh->GetRenderData().Bones[BoneIndex].BoneName;

    // 트리 노드 플래그 설정
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick| ImGuiTreeNodeFlags_DefaultOpen;

    // 현재 선택된 본인지 확인
    bool isSelected = (SelectedBoneIndex == BoneIndex);
    if (isSelected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
        // 선택된 본의 배경색 변경
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));         // 파란색 배경
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));  // 마우스 오버 시 밝은 파란색
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));   // 클릭 시 어두운 파란색
    }

    // 자식이 없는 본은 리프 노드로 표시
    if (SkeletalMesh->GetSkeleton()->GetRefSkeletal()->BoneTree[BoneIndex].ChildIndices.Num() == 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    // 회전 적용 여부에 따라 표시 이름 설정
    bool isModified = false;
    FBoneRotation* foundRotation = BoneRotations.Find(BoneIndex);
    if (foundRotation &&
        (foundRotation->X != 0.0f || foundRotation->Y != 0.0f || foundRotation->Z != 0.0f))
    {
        isModified = true;
    }

    // 트리 노드 표시
    bool isOpen = false;
    if (isModified)
    {
        // 수정된 본은 주황색 텍스트로 표시
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
        isOpen = ImGui::TreeNodeEx((void*)(intptr_t)BoneIndex, flags, "%s [Modified]", GetData(boneName));
        ImGui::PopStyleColor(); // Text 색상 복원
    }
    else
    {
        isOpen = ImGui::TreeNodeEx((void*)(intptr_t)BoneIndex, flags, "%s", GetData(boneName));
    }

    // 선택된 본의 스타일 색상 복원
    if (isSelected)
    {
        ImGui::PopStyleColor(3); // 스타일 색상 3개 복원 (Header, HeaderHovered, HeaderActive)
    }
    if (ImGui::BeginPopupContextItem(
            /* str_id = */ GetData(boneName),              // nullptr 로 하면 last item 을 자동으로 잡습니다.
            /* flags  = */ ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Add Sphere Collision"))
            AddSphereToBone(boneName);
        if (ImGui::MenuItem("Add Box Collision"))
             AddBoxToBone(boneName);
        if (ImGui::MenuItem("Add Capsule Collision"))
             AddCapsuleToBone(boneName);
        if (ImGui::MenuItem("Add Convex Collision"))
            AddConvexToBone(boneName);
        ImGui::EndPopup();
    }

    // 노드가 클릭되었는지 확인
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        // 클릭 시 실행할 함수 호출
        OnBoneSelected(BoneIndex);
    }

    // 트리 노드가 열려있으면 자식 노드들을 재귀적으로 렌더링
    if (isOpen)
    {
        if (PhysicsAsset)
        {
            for (UBodySetup* BodySetup : PhysicsAsset->BodySetup)
            {
                if (BodySetup && BodySetup->BoneName == boneName)
                {
                    if (ImGui::TreeNode("%s",GetData(boneName)))
                    {
                        if (ImGui::IsItemClicked())
                        {
                            SelectedBodySetup = BodySetup;
                        }

                        const FKAggregateGeom& Agg = BodySetup->AggGeom;

                        // 콜리전 프리미티브 출력
                        for (const auto& S : Agg.SphereElems)
                            ImGui::BulletText("Sphere  R=%.2f  Off=(%.2f,%.2f,%.2f)", S.Radius, S.Center.X, S.Center.Y, S.Center.Z);
                        for (const auto& B : Agg.BoxElems)
                            ImGui::BulletText("Box     E=(%.2f,%.2f,%.2f) Off=(%.2f,%.2f,%.2f)", B.X, B.Y, B.Z, B.Center.X, B.Center.Y, B.Center.Z);
                        for (const auto& Sy : Agg.SphylElems)
                            ImGui::BulletText("Capsule R=%.2f  L=%.2f  Off=(%.2f,%.2f,%.2f)", Sy.Radius, Sy.Length, Sy.Center.X, Sy.Center.Y, Sy.Center.Z);
                        for (const auto& C : Agg.ConvexElems)
                            ImGui::BulletText("Convex  Verts=%d", C.VertexData.Num());

                        ImGui::TreePop();
                    }
                    break; // 하나만 매칭되면 탈출
                }
            }
        }
        // 모든 자식 본 표시
        for (int32 ChildIndex : SkeletalMesh->GetSkeleton()->GetRefSkeletal()->BoneTree[BoneIndex].ChildIndices)
        {
            RenderBoneHierarchy(SkeletalMesh, ChildIndex);
        }
        ImGui::TreePop();
    }
}
bool FPhysicsSkeletonPanel::CheckAndCreateBodySetup(const FName& BoneName, UBodySetup*& FoundSetup)
{
    if (!PhysicsAsset) return false;

    FoundSetup = nullptr;
    for (UBodySetup* Setup : PhysicsAsset->BodySetup)
    {
        if (Setup && Setup->BoneName == BoneName)
        {
            FoundSetup = Setup;
            break;
        }
    }
    if (!FoundSetup)
    {
        FoundSetup = FObjectFactory::ConstructObject<UBodySetup>(PhysicsAsset);
        FoundSetup->BoneName = BoneName;
        PhysicsAsset->BodySetup.Add(FoundSetup);
    }
    return true;
}

void FPhysicsSkeletonPanel::AddSphereToBone(const FName& BoneName)
{
    UBodySetup* FoundSetup;
    if (!CheckAndCreateBodySetup(BoneName, FoundSetup)) return;

    // 2) 없으면 새로 생성
    if (!FoundSetup)
    {
        FoundSetup = FObjectFactory::ConstructObject<UBodySetup>(PhysicsAsset);
        FoundSetup->BoneName = BoneName;
        PhysicsAsset->BodySetup.Add(FoundSetup);
        // PhysicsAsset->MarkPackageDirty();
    }

    // 3) AggGeom에 SphereElem 추가
    FKSphereElem NewSphere;
    NewSphere.Radius = 10.0f;                // 기본값: 반경 10
    NewSphere.Center = FVector::ZeroVector;  // 본 로컬 오프셋
    FoundSetup->AggGeom.SphereElems.Add(NewSphere);

    // PhysicsAsset->MarkPackageDirty();

    // 4) UI 동기화(필요 시)
    // bNeedsRefresh = true;
}
void FPhysicsSkeletonPanel::AddBoxToBone(const FName& BoneName)
{
    UBodySetup* FoundSetup;
    if (!CheckAndCreateBodySetup(BoneName, FoundSetup)) return;

    // 2) 없으면 새로 생성
    if (!FoundSetup)
    {
        FoundSetup = FObjectFactory::ConstructObject<UBodySetup>(PhysicsAsset);
        FoundSetup->BoneName = BoneName;
        PhysicsAsset->BodySetup.Add(FoundSetup);
    }

    // 3) AggGeom에 BoxElem 추가
    FKBoxElem NewBox;
    NewBox.X = 10.0f;                      // 기본 박스 절반 크기 X
    NewBox.Y = 10.0f;                      // 기본 박스 절반 크기 Y
    NewBox.Z = 10.0f;                      // 기본 박스 절반 크기 Z
    NewBox.Center = FVector::ZeroVector;   // 로컬 오프셋
    FoundSetup->AggGeom.BoxElems.Add(NewBox);

    // 4) 저장 플래그
    // PhysicsAsset->MarkPackageDirty();

    // 5) UI 리프레시
    // bNeedsRefresh = true;
}

void FPhysicsSkeletonPanel::AddCapsuleToBone(const FName& BoneName)
{
    UBodySetup* FoundSetup;
    if (!CheckAndCreateBodySetup(BoneName, FoundSetup)) return;

    // FKSphylElem: Radius 와 HalfHeight 세팅
    FKSphylElem NewCapsule;
    NewCapsule.Radius     = 5.0f;           // 캡슐 반지름
    NewCapsule.Length     = 20.0f;          // 캡슐 길이 (전체 길이에서 반지름 제외한 축 방향 절반길이)
    NewCapsule.Center     = FVector::ZeroVector;
    FoundSetup->AggGeom.SphylElems.Add(NewCapsule);

    // PhysicsAsset->MarkPackageDirty();
    // bNeedsRefresh = true;
}
void FPhysicsSkeletonPanel::AddConvexToBone(const FName& BoneName)
{
    UBodySetup* FoundSetup;
    if (!CheckAndCreateBodySetup(BoneName, FoundSetup)) return;

    // FKConvexElem: 기본 빈 컨벡스 메시 추가
    FKConvexElem NewConvex;
    // 최소한 하나의 점이라도 있어야 내부적으로 유효할 수 있으니, 임시로 원점 하나 추가
    NewConvex.VertexData.Add(FVector::ZeroVector);
    // ConvexElem.Transform, Rotation, Scale 등 필요 시 세팅 가능
    FoundSetup->AggGeom.ConvexElems.Add(NewConvex);
    
}
// 뼈가 선택되었을 때 호출되는 함수
void FPhysicsSkeletonPanel::OnBoneSelected(int BoneIndex)
{
    SelectedBoneIndex = BoneIndex;
}

void FPhysicsSkeletonPanel::RenderShapeProperty(AActor* PickedActor)
{
    if (PickedActor && PickedComponent && PickedComponent->IsA<UBoxShapeComponent>())
    {
        UBoxShapeComponent* ShapeComp = Cast<UBoxShapeComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("BoxShapeComponent", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            FVector BoxExtent = ShapeComp->GetBoxExtent();

            if (FImGuiWidget::DrawVec3Control("BoxExtent", BoxExtent, 0, 10))
            {
                ShapeComp->SetBoxExtent(BoxExtent);
            }

            ImGui::TreePop();
        }
    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<USphereShapeComponent>())
    {
        USphereShapeComponent* ShapeComp = Cast<USphereShapeComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("SphereShapeComponent", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            float Radius = ShapeComp->GetRadius();

            if (ImGui::SliderFloat("Radius", &Radius, 0.0f, 100.0f))
            {
                ShapeComp->SetRadius(Radius);
            }

            ImGui::TreePop();
        }
    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<UCapsuleShapeComponent>())
    {
        UCapsuleShapeComponent* ShapeComp = Cast<UCapsuleShapeComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("CapsuleShapeComponent", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            float CapsuleRaidus = ShapeComp->GetRadius();

            if (ImGui::SliderFloat("CapsuleRaidus", &CapsuleRaidus, 0.0f, 100.0f))
            {
                ShapeComp->SetRadius(CapsuleRaidus);
            }

            float CapsuleHalfHeight = ShapeComp->GetHalfHeight();

            if (ImGui::SliderFloat("CapsuleHalfHeight", &CapsuleHalfHeight, 0.0f, 100.0f))
            {
                ShapeComp->SetHalfHeight(CapsuleHalfHeight);
            }

            ImGui::TreePop();
        }
    }
}

void FPhysicsSkeletonPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
    PhysicsDetailPanel.OnResize(hWnd);
}
