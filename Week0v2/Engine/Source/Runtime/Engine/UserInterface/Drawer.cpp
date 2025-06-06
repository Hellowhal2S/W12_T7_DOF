﻿#include "Drawer.h"

// #include "Engine/FbxLoader.h"
#include "LaunchEngineLoop.h"
#include "Actors/SkeletalMeshActor.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Engine/FEditorStateManager.h"
#include "ImGui/imgui_internal.h"
#include "UObject/UObjectIterator.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Skeletal/SkeletalDefine.h"
#include "UnrealEd/PhysicsPreviewUI.h"
#include "UnrealEd/UnrealEd.h"

void FDrawer::Toggle()
{
    bIsOpen = !bIsOpen;
    if (!bFirstOpenFrame)
    {
        bFirstOpenFrame = true;
    }
}

void FDrawer::Render()
{
}

void FDrawer::Render(float DeltaTime)
{
    if (!bIsOpen)
        return;

    ImVec2 WinSize = ImVec2(Width, Height);

    // 목표 위치
    float TargetY = WinSize.y * 0.75f;
    float StartY  = WinSize.y; // 아래에서 올라오게
    float CurrentY = TargetY;

    if (bFirstOpenFrame)
    {
        AnimationTime = 0.0f;
        bFirstOpenFrame = false;
    }

    // 애니메이션 진행
    if (AnimationTime < AnimationDuration)
    {
        AnimationTime += DeltaTime;
        float T = AnimationTime / AnimationDuration;
        T = ImClamp(T, 0.0f, 1.0f);
        T = ImGui::GetStyle().Alpha * T; // 곡선 보간을 원한다면 여기서 ease 적용
        CurrentY = ImLerp(StartY, TargetY, T);
    }

    ImGui::SetNextWindowPos(ImVec2(5, CurrentY));
    ImGui::SetNextWindowSize(ImVec2(WinSize.x - 10.0f, WinSize.y * 0.25f));
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Content Drawer", nullptr, PanelFlags);
    RenderContentDrawer();
    ImGui::End();
}

void FDrawer::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void FDrawer::RenderContentDrawer()
{
    int count =0;
    for (auto Obj : TObjectRange<USkeletalMesh>())
    {
        count++;
        if (Obj->GetOuter() != nullptr)
            continue;
        ImGui::Selectable(GetData(Obj->GetRenderData().Name));
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            UWorld* World =  Cast<UEditorEngine>(GEngine)->CreatePreviewWindow("PhysicsViewer", EWorldType::EditorPhysicsPreview);

            // SkeletalMeshActor 생성
            ASkeletalMeshActor* SkeletalMeshActor = World->SpawnActor<ASkeletalMeshActor>();
            SkeletalMeshActor->SetActorLabel("PreviewSkeletalMeshActor");

            // Mesh 및 Animation 설정
            USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(SkeletalMeshActor->GetRootComponent());
            // SkeletalMeshComponent->SetSkeletalMesh(FFBXLoader::CreateSkeletalMesh(Obj->GetRenderData().Name));
            SkeletalMeshComponent->SetSkeletalMesh(Obj);
            Cast<UEditorEngine>(GEngine)->GetPhysicsPreviewUI()->SetSkeletalMesh(Obj);
            break;
        }
    }
    UE_LOG(LogLevel::Warning,"USkeletal Count : %d", count);
}
