#include "SkeletonPanel.h"

#include <shellapi.h> // ShellExecute 관련 함수 정의 포함

#include "ImGUI/imgui.h"

#include "tinyfiledialogs/tinyfiledialogs.h"

#include "Engine/World.h"
#include "Engine/FLoaderOBJ.h"
#include "UnrealEd/ImGuiWidget.h"

#include "Math/JungleMath.h"

#include "Components/GameFramework/ProjectileMovementComponent.h"
#include "Components/GameFramework/RotatingMovementComponent.h"
#include "Components/LuaComponent.h"
#include "Components/LightComponents/DirectionalLightComponent.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/HeightFogComponent.h"
#include "Components/PrimitiveComponents/UParticleSubUVComp.h"
#include "Components/PrimitiveComponents/UTextComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/CubeComp.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"

#include "LevelEditor/SLevelEditor.h"

#include "LaunchEngineLoop.h"
#include "PlayerCameraManager.h"
#include "Engine/FBXLoader.h"
#include "Animation/Skeleton.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Light/ShadowMapAtlas.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/FunctionRegistry.h"

void FSkeletonPanel::Initialize(float InWidth, float InHeight)
{
    Width = InWidth;
    Height = InHeight;
}

void FSkeletonPanel::Render()
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (EditorEngine == nullptr)
    {
        return;
    }
    
    // TODO PickedComponent 패널에서 뺴기 우선 임시용으로 배치
    if ((GetAsyncKeyState(VK_DELETE) & 0x8000))
    {
        if (PickedComponent != nullptr)
        {
            if (World->GetSelectedActors().IsEmpty() || !World->GetSelectedActors().Contains(PickedComponent->GetOwner()))
            {
                PickedComponent = nullptr;
            }
        }
    }
    /* Pre Setup */
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
    ImGui::Begin("Skeleton", nullptr, PanelFlags);

    // 프리뷰 월드에서는 오로지 하나의 액터만 선택 가능 (스켈레탈 메쉬 프리뷰인 경우, 스켈레탈 메쉬 액터)
    AActor* PickedActor = nullptr;

    if (!World->GetSelectedActors().IsEmpty())
    {
        PickedActor = *World->GetSelectedActors().begin();
    }

    ImVec2 imageSize = ImVec2(256, 256); // 이미지 출력 크기
    

    if (PickedActor) // Delegate Test
    {
        RenderDelegate(World->GetLevel());
    }

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
            RenderForSkeletalMesh2(SkeletalMeshComponet);
        }
    }
    

    RenderShapeProperty(PickedActor);

    ImGui::End();
    PhysicsDetailPanel.Render();
    ImGui::PopStyleColor();
}

void FSkeletonPanel::DrawSceneComponentTree(USceneComponent* Component, UActorComponent*& PickedComponent)
{
    if (!Component) return;

    FString Label = *Component->GetName();
    bool bSelected = (PickedComponent == Component);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (bSelected)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    // 노드를 클릭 가능한 셀렉션으로 표시
    bool bOpened = ImGui::TreeNodeEx(*Label, nodeFlags);

    // 클릭되었을 때 선택 갱신
    if (ImGui::IsItemClicked())
    {
        PickedComponent = Component;
    }

    // 자식 재귀 호출
    if (bOpened)
    {
        for (USceneComponent* Child : Component->GetAttachChildren())
        {
            DrawSceneComponentTree(Child, PickedComponent);
        }
        ImGui::TreePop();
    }
}

void FSkeletonPanel::DrawActorComponent(UActorComponent* Component, UActorComponent*& PickedComponent)
{
    if (!Component) return;

    FString Label = *Component->GetName();
    bool bSelected = (PickedComponent == Component);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (bSelected)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    if (ImGui::Selectable(*Label, nodeFlags))
    {
        PickedComponent = Component;
    }
}

void FSkeletonPanel::RGBToHSV(float r, float g, float b, float& h, float& s, float& v) const
{
    float mx = FMath::Max(r, FMath::Max(g, b));
    float mn = FMath::Min(r, FMath::Min(g, b));
    float delta = mx - mn;

    v = mx;

    if (mx == 0.0f) {
        s = 0.0f;
        h = 0.0f;
        return;
    }
    else {
        s = delta / mx;
    }

    if (delta < 1e-6) {
        h = 0.0f;
    }
    else {
        if (r >= mx) {
            h = (g - b) / delta;
        }
        else if (g >= mx) {
            h = 2.0f + (b - r) / delta;
        }
        else {
            h = 4.0f + (r - g) / delta;
        }
        h *= 60.0f;
        if (h < 0.0f) {
            h += 360.0f;
        }
    }
}

void FSkeletonPanel::HSVToRGB(float h, float s, float v, float& r, float& g, float& b) const
{
    // h: 0~360, s:0~1, v:0~1
    float c = v * s;
    float hp = h / 60.0f;             // 0~6 구간
    float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float m = v - c;

    if (hp < 1.0f) { r = c;  g = x;  b = 0.0f; }
    else if (hp < 2.0f) { r = x;  g = c;  b = 0.0f; }
    else if (hp < 3.0f) { r = 0.0f; g = c;  b = x; }
    else if (hp < 4.0f) { r = 0.0f; g = x;  b = c; }
    else if (hp < 5.0f) { r = x;  g = 0.0f; b = c; }
    else { r = c;  g = 0.0f; b = x; }

    r += m;  g += m;  b += m;
}

void FSkeletonPanel::RenderForSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp)
{
    if (SkeletalMeshComp->GetSkeletalMesh() == nullptr)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Skeletal Mesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("Skeletal Mesh");

        std::vector<std::string> fbxFiles;
        static const std::string folder = std::filesystem::current_path().string() + "/Contents/FBX";
        for (auto& entry : std::filesystem::directory_iterator(folder))
        {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() == ".fbx")
                fbxFiles.push_back(entry.path().filename().string());
        }

        static int currentIndex = 0;
        const char* preview = fbxFiles.empty() 
            ? "No .fbx files" 
            : fbxFiles[currentIndex].c_str();

        FString PreviewName = SkeletalMeshComp->GetSkeletalMesh()->GetRenderData().Name;
        std::filesystem::path P = PreviewName;
        FString FileName = FString( P.filename().string() ); 
        
        const TMap<FString, USkeletalMesh*> Meshes = FFBXLoader::GetSkeletalMeshes();
        if (ImGui::BeginCombo("##SkeletalMesh", GetData(FileName), ImGuiComboFlags_None))
        {
            for (int i = 0; i < (int)fbxFiles.size(); ++i)
            {
                bool isSelected = (i == currentIndex);
                if (ImGui::Selectable(fbxFiles[i].c_str(), isSelected))
                {
                    currentIndex = i;
                    std::string fullPath = "FBX/" + fbxFiles[i];
                    SkeletalMeshComp->LoadSkeletalMesh(fullPath);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        for (const auto& Bone : SkeletalMeshComp->GetSkeletalMesh()->GetSkeleton()->GetRefSkeletal()->BoneTree)
        {
            for (const auto& RootBoneIndex : SkeletalMeshComp->GetSkeletalMesh()->GetSkeleton()->GetRefSkeletal()->RootBoneIndices)
            {
                if (Bone.BoneIndex == RootBoneIndex)
                {
                    RenderBoneHierarchy(SkeletalMeshComp->GetSkeletalMesh(), Bone.BoneIndex);
                }
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}
void FSkeletonPanel::RenderForSkeletalMesh2(USkeletalMeshComponent* SkeletalMesh)
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

void FSkeletonPanel::RenderBoneHierarchy(USkeletalMesh* SkeletalMesh, int32 BoneIndex)
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

    // 노드가 클릭되었는지 확인
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        // 클릭 시 실행할 함수 호출
        OnBoneSelected(BoneIndex);
    }

    // 트리 노드가 열려있으면 자식 노드들을 재귀적으로 렌더링
    if (isOpen)
    {
        // 모든 자식 본 표시
        for (int32 ChildIndex : SkeletalMesh->GetSkeleton()->GetRefSkeletal()->BoneTree[BoneIndex].ChildIndices)
        {
            RenderBoneHierarchy(SkeletalMesh, ChildIndex);
        }

        ImGui::TreePop();
    }
}

// 뼈가 선택되었을 때 호출되는 함수
void FSkeletonPanel::OnBoneSelected(int BoneIndex)
{
    SelectedBoneIndex = BoneIndex;
}

void FSkeletonPanel::RenderShapeProperty(AActor* PickedActor)
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

void FSkeletonPanel::RenderDelegate(ULevel* level)
{
    static AActor* SelectedActor = nullptr;
    FString SelectedActorName;
    SelectedActor ? SelectedActorName = SelectedActor->GetName() : SelectedActorName = "";
    
    if (ImGui::BeginCombo("Delegate Object", GetData(SelectedActorName), ImGuiComboFlags_None))
    {
        for (const auto& Actor : level->GetActors())
        {
            if (ImGui::Selectable(GetData(Actor->GetName()), false))
            {
                SelectedActor = Actor;
            }
        }
        ImGui::EndCombo();
    }
    
    static FString SelectedFunctionName = "";
    if (SelectedActor)
    {
        if (ImGui::BeginCombo("Delegate Function", GetData(SelectedFunctionName), ImGuiComboFlags_None))
        {
            for (const auto& function : SelectedActor->FunctionRegistry()->GetRegisteredFunctions())
            {
                if (ImGui::Selectable(GetData(function.Key.ToString()), false))
                {
                    SelectedFunctionName = function.Key.ToString();
                }
            }
            ImGui::EndCombo();
        }
    }
}

void FSkeletonPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
    PhysicsDetailPanel.OnResize(hWnd);
}
