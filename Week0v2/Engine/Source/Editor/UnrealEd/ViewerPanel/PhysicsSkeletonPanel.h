#pragma once

#pragma once
#include "Define.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UnrealEd/EditorPanel.h"
#include "UnrealEd/ViewerPanel/SkeletonDetailPanel.h"

struct FBoneRotation
{
    float X;
    float Y;
    float Z;
    
    FBoneRotation() : X(0.0f), Y(0.0f), Z(0.0f) {}
    FBoneRotation(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
};

class AActor;
class USkeletalMeshComponent;
class UActorComponent;
class UStaticMeshComponent;
class USceneComponent;
class ULevel;

class FPhysicsSkeletonPanel : public UEditorPanel
{
public:
    void Initialize(float InWidth, float InHeight);
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:

    /* Static Mesh Settings */
    void RenderForSkeletalMesh(USkeletalMeshComponent* SkeletalMesh);
    void RenderBoneHierarchy(USkeletalMesh* SkeletalMesh, int BoneIndex);
    void OnBoneSelected(int BoneIndex);
    
    void RenderShapeProperty(AActor* PickedActor);

    void AddSphereToBone(const FName& BoneName);
private:
    float Width = 0, Height = 0;
    
    /* Material Property */
    int SelectedMaterialIndex = -1;
    int CurMaterialIndex = -1;
    UStaticMeshComponent* SelectedStaticMeshComp = nullptr;
    USkeletalMeshComponent* SelectedSkeletalMeshComp = nullptr;
    int SelectedBoneIndex = -1;

    FObjMaterialInfo tempMaterialInfo;
    bool IsCreateMaterial;
    UActorComponent* PickedComponent = nullptr;
    UActorComponent* LastComponent = nullptr;
    bool bFirstFrame = true;

    UPhysicsAsset* PhysicsAsset = nullptr;

    //FBX
private:
    float XRotation = 0.0f;
    float YRotation = 0.0f;
    float ZRotation = 0.0f;
    
    TMap<int, FBoneRotation> BoneRotations;
    TMap<int, FBoneRotation> PrevBoneRotations;

    FSkeletonDetailPanel PhysicsDetailPanel;
};


