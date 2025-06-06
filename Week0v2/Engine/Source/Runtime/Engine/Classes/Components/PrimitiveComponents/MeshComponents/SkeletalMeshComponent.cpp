#include "SkeletalMeshComponent.h"

#include "LaunchEngineLoop.h"
#include "Engine/FBXLoader.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Engine/World.h"
#include "Launch/EditorEngine.h"
#include "UObject/ObjectFactory.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Classes/Engine/FLoaderOBJ.h"
#include "GameFramework/Actor.h"
#include "Math/JungleMath.h"
#include "Math/Transform.h"
#include "Renderer/Renderer.h"
#include "StaticMeshComponents/StaticMeshComponent.h"
#include "UObject/Casts.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/CustomAnimInstance/TestAnimInstance.h"
#include "Engine/FEditorStateManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Physics/PhysX.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PhysicsEngine/ConstraintSetup.h"
#include "PhysicsEngine/JointElem.h"

USkeletalMeshComponent::USkeletalMeshComponent(const USkeletalMeshComponent& Other)
    : UMeshComponent(Other)
    , SkeletalMesh(Other.SkeletalMesh)
    , SelectedSubMeshIndex(Other.SelectedSubMeshIndex)
{

}
uint32 USkeletalMeshComponent::GetNumMaterials() const
{
    if (SkeletalMesh == nullptr) return 0;

    return SkeletalMesh->GetMaterials().Num();
}

UMaterial* USkeletalMeshComponent::GetMaterial(uint32 ElementIndex) const
{
    if (SkeletalMesh != nullptr)
    {
        if (OverrideMaterials[ElementIndex] != nullptr)
        {
            return OverrideMaterials[ElementIndex];
        }
    
        if (SkeletalMesh->GetMaterials().IsValidIndex(ElementIndex))
        {
            return SkeletalMesh->GetMaterials()[ElementIndex]->Material;
        }
    }
    return nullptr;
}

uint32 USkeletalMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
    if (SkeletalMesh == nullptr) return -1;

    return SkeletalMesh->GetMaterialIndex(MaterialSlotName);
}

TArray<FName> USkeletalMeshComponent::GetMaterialSlotNames() const
{
    TArray<FName> MaterialNames;
    if (SkeletalMesh == nullptr) return MaterialNames;

    for (const FMaterialSlot* Material : SkeletalMesh->GetMaterials())
    {
        MaterialNames.Emplace(Material->MaterialSlotName);
    }

    return MaterialNames;
}

void USkeletalMeshComponent::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    if (SkeletalMesh == nullptr) return;
    SkeletalMesh->GetUsedMaterials(Out);
    for (int materialIndex = 0; materialIndex < GetNumMaterials(); materialIndex++)
    {
        if (OverrideMaterials[materialIndex] != nullptr)
        {
            Out[materialIndex] = OverrideMaterials[materialIndex];
        }
    }
}

int USkeletalMeshComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    if (!AABB.IntersectRay(rayOrigin, rayDirection, pfNearHitDistance)) return 0;
    int nIntersections = 0;
    if (SkeletalMesh == nullptr) return 0;

    FSkeletalMeshRenderData renderData = SkeletalMesh->GetRenderData();

    FSkeletalVertex* vertices = renderData.Vertices.GetData();
    int vCount = renderData.Vertices.Num();
    UINT* indices = renderData.Indices.GetData();
    int iCount = renderData.Indices.Num();

    if (!vertices) return 0;
    BYTE* pbPositions = reinterpret_cast<BYTE*>(renderData.Vertices.GetData());

    int nPrimitives = (!indices) ? (vCount / 3) : (iCount / 3);
    float fNearHitDistance = FLT_MAX;
    for (int i = 0; i < nPrimitives; i++)
    {
        int idx0, idx1, idx2;
        if (!indices)
        {
            idx0 = i * 3;
            idx1 = i * 3 + 1;
            idx2 = i * 3 + 2;
        }
        else
        {
            idx0 = indices[i * 3];
            idx2 = indices[i * 3 + 1];
            idx1 = indices[i * 3 + 2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        uint32 stride = sizeof(FSkeletalVertex);
        FVector v0 = *reinterpret_cast<FVector*>(pbPositions + idx0 * stride);
        FVector v1 = *reinterpret_cast<FVector*>(pbPositions + idx1 * stride);
        FVector v2 = *reinterpret_cast<FVector*>(pbPositions + idx2 * stride);

        float fHitDistance;
        if (IntersectRayTriangle(rayOrigin, rayDirection, v0, v1, v2, fHitDistance))
        {
            if (fHitDistance < fNearHitDistance)
            {
                pfNearHitDistance = fNearHitDistance = fHitDistance;
            }
            nIntersections++;
        }
    }
    return nIntersections;
}


void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* value)
{
    SkeletalMesh = value;
    VBIBTopologyMappingName = SkeletalMesh->GetFName();
    value->UpdateBoneHierarchy();

    OverrideMaterials.SetNum(value->GetMaterials().Num());
    AABB = SkeletalMesh->GetRenderData().BoundingBox;

    // CreateBoneComponents();
    // CreateRagdollBones();
    // CreateRagdoll(PxVec3(0, 0, 100));
    //CreateRagedollBodySetUp();
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const
{
    return Cast<UAnimSingleNodeInstance>(AnimInstance);
}

void USkeletalMeshComponent::CreateBoneComponents()
{
    // 이미 할당된 component가 있다면 삭제
    for (auto& BoneComp : BoneComponents)
    {
        BoneComp->DestroyComponent();
    }

    FManagerOBJ::CreateStaticMesh("Contents/helloBlender.obj");
    for (const auto& Bone : GetSkeletalMesh()->GetRenderData().Bones)
    {
        UStaticMeshComponent* BoneComp = GetOwner()->AddComponent<UStaticMeshComponent>(EComponentOrigin::Runtime);
        BoneComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"helloBlender.obj"));
        BoneComp->SetWorldLocation(Bone.GlobalTransform.GetTranslationVector());
        BoneComp->SetFName(Bone.BoneName);
        BoneComponents.Add(BoneComp);
    }
}

USkeletalMesh* USkeletalMeshComponent::LoadSkeletalMesh(const FString& FilePath)
{
    USkeletalMesh* SkeletalMesh = FFBXLoader::CreateSkeletalMesh(FilePath);
    SetSkeletalMesh(SkeletalMesh);

    return SkeletalMesh;
}

void USkeletalMeshComponent::UpdateBoneHierarchy()
{
    for (int i = 0; i < SkeletalMesh->GetRenderData().Vertices.Num(); i++)
    {
        SkeletalMesh->GetRenderData().Vertices[i].Position
            = SkeletalMesh->GetSkeleton()->GetRefSkeletal()->RawVertices[i].Position;
    }

    SkeletalMesh->UpdateBoneHierarchy();
    SkinningVertex();
}

void USkeletalMeshComponent::ReleaseBodies()
{
    PxScene* gScene = GetWorld()->GetPhysicsScene();
    for (FBodyInstance* Body : Bodies)
    {
        gScene->removeActor(*Body->RigidActorHandle);
        Body->Release();
        delete Body;
    }
    Bodies.Empty();
    
    for (FConstraintInstance* Constraint : Constraints)
    {
        Constraint->Release();
        delete Constraint;
    }
    Constraints.Empty();
}

void USkeletalMeshComponent::PlayAnimation(UAnimSequence* NewAnimToPlay, bool bLooping)
{
    SetAnimSequence(NewAnimToPlay);
    Play(bLooping);
}

void USkeletalMeshComponent::SetAnimSequence(UAnimSequence* NewAnimToPlay)
{
    if (NewAnimToPlay == nullptr)
    {
        return;
    }
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetCurrentSequence(NewAnimToPlay);
    }
}

UAnimSequence* USkeletalMeshComponent::GetAnimSequence() const
{
    return GetSingleNodeInstance()->GetCurrentSequence();
}

void USkeletalMeshComponent::SkinningVertex()
{
    for (auto& Vertex : SkeletalMesh->GetRenderData().Vertices)
    {
        Vertex.SkinningVertex(SkeletalMesh->GetRenderData().Bones);
    }

    FFBXLoader::UpdateBoundingBox(SkeletalMesh->GetRenderData());
    AABB = SkeletalMesh->GetRenderData().BoundingBox;

    SkeletalMesh->SetData(SkeletalMesh->GetRenderData(), SkeletalMesh->GetSkeleton()); // TODO: Dynamic VertexBuffer Update하게 바꾸기
}

// std::unique_ptr<FActorComponentInfo> USkeletalMeshComponent::GetComponentInfo()
// {
//     auto Info = std::make_unique<FStaticMeshComponentInfo>();
//     SaveComponentInfo(*Info);
//     
//     return Info;
// }

// void UStaticMeshComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
// {
//     FStaticMeshComponentInfo* Info = static_cast<FStaticMeshComponentInfo*>(&OutInfo);
//     Super::SaveComponentInfo(*Info);
//
//     Info->StaticMeshPath = staticMesh->GetRenderData()->PathName;
// }

// void UStaticMeshComponent::LoadAndConstruct(const FActorComponentInfo& Info)
// {
//     Super::LoadAndConstruct(Info);
//
//     const FStaticMeshComponentInfo& StaticMeshInfo = static_cast<const FStaticMeshComponentInfo&>(Info);
//     UStaticMesh* Mesh = FManagerOBJ::CreateStaticMesh(FString::ToFString(StaticMeshInfo.StaticMeshPath));
//     SetStaticMesh(Mesh);
// }

UObject* USkeletalMeshComponent::Duplicate(UObject* InOuter)
{
    USkeletalMeshComponent* NewComp = FObjectFactory::ConstructObjectFrom<USkeletalMeshComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();
    return NewComp;
}

void USkeletalMeshComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UMeshComponent::DuplicateSubObjects(Source, InOuter);
    // TODO: SkeletalMesh 복사
    SkeletalMesh = Cast<USkeletalMesh>(Cast<USkeletalMeshComponent>(Source)->SkeletalMesh->Duplicate(this));
    AnimInstance = Cast<UAnimInstance>(Cast<USkeletalMeshComponent>(Source)->AnimInstance->Duplicate(this));
}

void USkeletalMeshComponent::PostDuplicate()
{
}

void USkeletalMeshComponent::BeginPlay()
{
    UMeshComponent::BeginPlay();
    animTime = 0.f;
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    if (AnimInstance) AnimInstance->NativeUpdateAnimation(DeltaTime);

    SkeletalMesh->UpdateBoneHierarchy();
    if (GEngineLoop.Renderer.GetSkinningMode() == ESkinningType::CPU)
    {
        SkeletalMesh->UpdateSkinnedVertices();
        bCPUSkinned = true;
    }
    else
    {
        if (bCPUSkinned)
        {
            SkeletalMesh->ResetToOriginalPose();
            bCPUSkinned = false;
        }
    }
}

void USkeletalMeshComponent::SetData(const FString& FilePath)
{
    SkeletalMesh = LoadSkeletalMesh(FilePath);
}

void USkeletalMesh::ResetToOriginalPose()
{
    // 본 트랜스폼 복원
    for (int i = 0; i < Skeleton->GetRefSkeletal()->RawVertices.Num() && i < SkeletalMeshRenderData.Bones.Num(); i++)
    {
        // 로컬 트랜스폼 복원
        SkeletalMeshRenderData.Bones[i].LocalTransform = Skeleton->GetRefSkeletal()->RawBones[i].LocalTransform;
        SkeletalMeshRenderData.Bones[i].GlobalTransform = Skeleton->GetRefSkeletal()->RawBones[i].GlobalTransform;
        SkeletalMeshRenderData.Bones[i].SkinningMatrix = Skeleton->GetRefSkeletal()->RawBones[i].SkinningMatrix;
    }

    // 스키닝 적용
    UpdateSkinnedVertices();
}

void USkeletalMeshComponent::Play(bool bLooping)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(true);
        SingleNodeInstance->SetLooping(bLooping);
    }
}

void USkeletalMeshComponent::Stop()
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(false);
    }
}

void USkeletalMeshComponent::SetPlaying(bool bPlaying)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlaying(bPlaying);
    }
}

bool USkeletalMeshComponent::IsPlaying() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsPlaying();
    }

    return false;
}

void USkeletalMeshComponent::SetReverse(bool bIsReverse)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetReverse(bIsReverse);
    }
}

bool USkeletalMeshComponent::IsReverse() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsReverse();
    }
}

void USkeletalMeshComponent::SetPlayRate(float Rate)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetPlayRate(Rate);
    }
}

float USkeletalMeshComponent::GetPlayRate() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetPlayRate();
    }

    return 0.f;
}

void USkeletalMeshComponent::SetLooping(bool bIsLooping)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLooping(bIsLooping);
    }
}

bool USkeletalMeshComponent::IsLooping() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->IsLooping();
    }
    return false;
}

int USkeletalMeshComponent::GetCurrentKey() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetCurrentKey();
    }
    return 0;
}

void USkeletalMeshComponent::SetCurrentKey(int InKey)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetCurrentKey(InKey);
    }
}

void USkeletalMeshComponent::SetElapsedTime(float InElapsedTime)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetElapsedTime(InElapsedTime);
    }
}

float USkeletalMeshComponent::GetElapsedTime() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetElapsedTime();
    }
    return 0.f;
}

int32 USkeletalMeshComponent::GetLoopStartFrame() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetLoopStartFrame();
    }
    return 0;
}

void USkeletalMeshComponent::SetLoopStartFrame(int32 InLoopStartFrame)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLoopStartFrame(InLoopStartFrame);
    }
}

int32 USkeletalMeshComponent::GetLoopEndFrame() const
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        return SingleNodeInstance->GetLoopEndFrame();
    }
    return 0;
}

void USkeletalMeshComponent::SetLoopEndFrame(int32 InLoopEndFrame)
{
    if (UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance())
    {
        SingleNodeInstance->SetLoopEndFrame(InLoopEndFrame);
    }
}

void USkeletalMeshComponent::CreateRagdollBones()
{
    const TArray<FBone>& Bones = SkeletalMesh->GetRenderData().Bones;

    RagdollBones.Empty();
    RagdollBones.Reserve(Bones.Num());

    for (int i = 0; i < Bones.Num(); ++i)
    {
        const FBone& Bone = Bones[i];
        RagdollBone* ragBone = new RagdollBone();
        ragBone->name = Bone.BoneName;
        ragBone->parentIndex = Bone.ParentIndex;

        if (Bone.ParentIndex < 0)
        {
            ragBone->offset = PxVec3(0, 0, 0);
            ragBone->halfSize = PxVec3(0.0, 0.0, 0.0);
            ragBone->offset = PxVec3(Bone.GlobalTransform.GetTranslationVector().X,
                                     Bone.GlobalTransform.GetTranslationVector().Y,
                                     Bone.GlobalTransform.GetTranslationVector().Z);
        }
        else
        {
            FVector parentPos = Bones[Bone.ParentIndex].GlobalTransform.GetTranslationVector();
            FVector childPos = Bone.GlobalTransform.GetTranslationVector();
            FVector offset = childPos - parentPos;

            float length = offset.Magnitude();
            float radius = FMath::Clamp(length * 0.1f, 0.5f, 3.0f);
            float halfHeight = FMath::Max(length * 0.5f - radius, 0.1f);

            ragBone->offset = PxVec3(offset.X, offset.Y, offset.Z);
            ragBone->halfSize = PxVec3(radius, radius, halfHeight);
        }

        RagdollBones.Add(ragBone);
    }
}

void USkeletalMeshComponent::CreateRagdoll(const PxVec3& worldRoot)
{
    const TArray<FBone>& Bones = SkeletalMesh->GetRenderData().Bones;

    for (int childIndex = 0; childIndex < Bones.Num(); ++childIndex)
    {
        const FBone& childBone = Bones[childIndex];
        int parentIndex = childBone.ParentIndex;
        RagdollBone& bone = *RagdollBones[childIndex];

        if (parentIndex < 0)
        {
            PxVec3 offset = bone.offset + worldRoot;
            PxRigidDynamic* DummyBody = gPhysics->createRigidDynamic(PxTransform(offset));
            bone.body = DummyBody;
            GetWorld()->GetPhysicsScene()->addActor(*DummyBody);
            continue;
        }

        FVector parentPos = Bones[parentIndex].GlobalTransform.GetTranslationVector() + FVector(worldRoot.x, worldRoot.y, worldRoot.z);;
        FVector childPos = childBone.GlobalTransform.GetTranslationVector() + FVector(worldRoot.x, worldRoot.y, worldRoot.z);;
        FVector mid = (parentPos + childPos) * 0.5f;
        FVector dir = (childPos - parentPos).GetSafeNormal();

        FQuat rot = FQuat::FindBetweenNormals(FVector(1, 0, 0), dir);
        PxQuat pxRot(rot.X, rot.Y, rot.Z, rot.W);

        float radius = bone.halfSize.x;
        float halfHeight = bone.halfSize.z;
        PxTransform capsulePose(PxVec3(mid.X, mid.Y, mid.Z), pxRot);

        PxCapsuleGeometry capsule(radius, halfHeight);
        PxShape* shape = gPhysics->createShape(capsule, *gMaterial);

        PxFilterData fd = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Environment, FPhysX::ECollisionGroup::All);
        shape->setSimulationFilterData(fd);

        PxRigidDynamic* body = gPhysics->createRigidDynamic(capsulePose);
        body->attachShape(*shape);

        PxRigidBodyExt::updateMassAndInertia(*body, 1.0f);

        GetWorld()->GetPhysicsScene()->addActor(*body);
        bone.body = body;
    }

    for (int childIndex = 0; childIndex < Bones.Num(); ++childIndex)
    {
        int parentIndex = Bones[childIndex].ParentIndex;
        if (parentIndex < 0) continue;

        RagdollBone& child = *RagdollBones[childIndex];
        RagdollBone& parent = *RagdollBones[parentIndex];

        if (child.body == nullptr || parent.body == nullptr)
            continue;

        // Use the child bone's position as anchor
        FVector anchorF = Bones[parentIndex].GlobalTransform.GetTranslationVector() + FVector(worldRoot.x, worldRoot.y, worldRoot.z);;
        PxVec3 anchorPos = PxVec3(anchorF.X, anchorF.Y, anchorF.Z);

        PxTransform localParent = parent.body->getGlobalPose().getInverse() * PxTransform(anchorPos);
        PxTransform localChild = child.body->getGlobalPose().getInverse() * PxTransform(anchorPos);

        PxD6Joint* joint = PxD6JointCreate(*gPhysics, parent.body, localParent, child.body, localChild);
        joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
        joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
        joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
        joint->setTwistLimit(PxJointAngularLimitPair(-PxPi / 4, PxPi / 4));
        joint->setSwingLimit(PxJointLimitCone(PxPi / 6, PxPi / 6));

        child.joint = joint;
    }
}

void USkeletalMeshComponent::CreateRagedollBodySetUp()
{
    if (!SkeletalMesh || !SkeletalMesh->GetPhysicsAsset())
        return;

    TArray<FBone>& Bones = SkeletalMesh->GetRenderData().Bones;
    UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();
    PhysicsAsset->BodySetup.Empty();

    for (int i = 0; i < Bones.Num(); ++i)
    {
        FBone& Bone = Bones[i];
        FName BoneName = Bone.BoneName;

        UBodySetup* BodySetup = FObjectFactory::ConstructObject<UBodySetup>(PhysicsAsset);
        BodySetup->BoneName = BoneName;
        PhysicsAsset->BodySetup.Add(BodySetup);

        if (Bone.ParentIndex >= 0)
        {
            FBone& ParentBone = Bones[Bone.ParentIndex];
            const FVector parentPosWS = ParentBone.GlobalTransform.GetTranslationVector();
            const FVector childPosWS = Bone.GlobalTransform.GetTranslationVector();
            FVector offsetWS = childPosWS - parentPosWS;

            const float length = offsetWS.Magnitude();
            const float radius = FMath::Clamp(length * 0.2f, 0.1f, 2.0f);

            // 캡슐 길이 계산 (실린더 부분만 고려, 반구는 별도)
            const float cylinderLength = FMath::Max(length - radius * 2, 0.1f); // 최소 길이 보장
            const float halfHeight = cylinderLength * 0.5f;

            // 부모 본의 로컬 공간에서의 방향 계산
            const FVector localOffset = ParentBone.GlobalTransform.InverseTransformVector(offsetWS);
            const FVector localDir = localOffset.GetSafeNormal();

            // 로컬 공간에서 Z축을 localDir로 회전
            const FQuat localRotation = FQuat::FindBetweenNormals(FVector(0, 0, 1), localDir);

            // 위치: 부모에서 자식 방향으로 radius만큼 떨어진 지점을 시작점으로 계산
            FVector capsuleStartWS = parentPosWS;
            FVector capsuleEndWS = childPosWS;
            FVector midPointWS = (capsuleStartWS + capsuleEndWS) * 0.5f;

            const FVector localCenter = ParentBone.GlobalTransform.InverseTransformPosition(midPointWS);

            // 캡슐 저장
            FKSphylElem capsule;
            capsule.Radius = radius;
            capsule.Length = cylinderLength; // 실린더 부분만의 길이
            capsule.Center = localCenter;
            capsule.Rotation = localRotation.Rotator();

            BodySetup->AggGeom.SphylElems.Add(capsule);
        }
    }
}

void USkeletalMeshComponent::CreateRagdollConstrinatSetup()
{
    if (!SkeletalMesh || !SkeletalMesh->GetPhysicsAsset())
        return;
    
    TArray<FBone>& Bones = SkeletalMesh->GetRenderData().Bones;
    UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();
    PhysicsAsset->ConstraintSetup.Empty();
    
    for (int i = 0; i < PhysicsAsset->BodySetup.Num(); ++i)
    {
        FBone& Bone = Bones[i];

        if (Bone.ParentIndex < 0)
            continue;
        
        UConstraintSetup* ConstraintSetup = FObjectFactory::ConstructObject<UConstraintSetup>(PhysicsAsset);
        ConstraintSetup->JointName = Bones[Bone.ParentIndex].BoneName + "-> " +  Bone.BoneName;
        PhysicsAsset->ConstraintSetup.Add(ConstraintSetup);
        
        FKJointElem joint;
        joint.ChildBoneName = Bone.BoneName;
        joint.ParentBoneName = Bones[Bone.ParentIndex].BoneName;
        
        joint.Center = Bones[Bone.ParentIndex].GlobalTransform.GetTranslationVector();
        joint.Rotation = FQuat(Bones[Bone.ParentIndex].GlobalTransform).GetSafeNormal();
        
        // 각도 제한
        joint.TwistLimitMin = -PxPi / 4;
        joint.TwistLimitMax = PxPi / 4;
        joint.SwingLimitMin1 = -PxPi / 6;
        joint.SwingLimitMax1 = PxPi / 6;
        joint.SwingLimitMin2 = -PxPi / 6;
        joint.SwingLimitMax2 = PxPi / 6;

        // 축 제한
        joint.AxisMotions[PxD6Axis::eX]      = EJointMotion::Locked;
        joint.AxisMotions[PxD6Axis::eY]      = EJointMotion::Locked;
        joint.AxisMotions[PxD6Axis::eZ]      = EJointMotion::Locked;
        joint.AxisMotions[PxD6Axis::eTWIST]  = EJointMotion::Limited;
        joint.AxisMotions[PxD6Axis::eSWING1] = EJointMotion::Limited;
        joint.AxisMotions[PxD6Axis::eSWING2] = EJointMotion::Limited;
        
        ConstraintSetup->JointElem = joint;
    }

}

void USkeletalMeshComponent::InstantiatePhysicsAssetBodies_Internal()
{
    Bodies.Empty();
    UPhysicsAsset* PhysicsAsset = GetSkeletalMesh()->GetPhysicsAsset();
    const auto& Bones = GetSkeletalMesh()->GetRenderData().Bones;
    const auto& RefSkeletal = GetSkeletalMesh()->GetSkeleton()->GetRefSkeletal();

    static const FQuat ToX = FQuat::FindBetweenNormals(FVector(0, 0, 1), FVector(1, 0, 0));

    for (const UBodySetup* BodySetup : PhysicsAsset->BodySetup)
    {
        int32* pIndex = RefSkeletal->BoneNameToIndexMap.Find(BodySetup->BoneName.ToString());
        if (!pIndex) continue;

        int32 ChildIndex = *pIndex;
        int32 ParentIndex = Bones[ChildIndex].ParentIndex;
        
        if (ParentIndex < 0)
        {
            FBodyInstance* Instance = new FBodyInstance(this, EBodyType::Dynamic, ToPxVec3(Bones[ChildIndex].GlobalTransform.GetTranslationVector()), BodySetup->BoneName);
            PxRigidBodyExt::updateMassAndInertia(*Instance->RigidDynamicHandle, 1.0f);
            Instance->RigidActorHandle->userData = (void*)Instance;
            GetWorld()->GetPhysicsScene()->addActor(*Instance->RigidDynamicHandle);
            Bodies.Add(Instance);
            continue;
        }

        // 부모 본의 월드 행렬
        const FMatrix ParentWorldTM = Bones[ParentIndex].GlobalTransform * GetWorldMatrix();
        const FVector globalPos = Bones[ParentIndex].GlobalTransform.GetTranslationVector();

        TArray<PxShape*> Shapes;

        // 3a. Sphere
        for (const FKSphereElem& S : BodySetup->AggGeom.SphereElems)
        {
            PxSphereGeometry geo(S.Radius);
            PxShape* shape = gPhysics->createShape(geo, *gMaterial);
            shape->setLocalPose(PxTransform(ToPxVec3(S.Center)));
            shape->setSimulationFilterData(FPhysX::MakeFilterData(
                FPhysX::ECollisionGroup::Environment,
                FPhysX::ECollisionGroup::All));
            Shapes.Add(shape);
        }

        // 3b. Box
        for (const FKBoxElem& B : BodySetup->AggGeom.BoxElems)
        {
            PxBoxGeometry geo(B.X * 0.5f, B.Y * 0.5f, B.Z * 0.5f);
            FQuat rot = B.Rotation.ToQuaternion();
            PxShape* shape = gPhysics->createShape(geo, *gMaterial);
            shape->setLocalPose(PxTransform(ToPxVec3(B.Center), ToPxQuat(rot)));
            shape->setSimulationFilterData(FPhysX::MakeFilterData(
                FPhysX::ECollisionGroup::Environment,
                FPhysX::ECollisionGroup::All));
            Shapes.Add(shape);
        }

        // 3c. Capsule (부모 기준 로컬 → 월드 변환)
        for (const FKSphylElem& C : BodySetup->AggGeom.SphylElems)
        {
            FVector centerWS = ParentWorldTM.TransformPosition(C.Center);
            FVector centerLocal = centerWS - globalPos;

            FQuat localRot = C.Rotation.ToQuaternion();
            FQuat worldRot = ParentWorldTM.ToQuat() * localRot;
            FQuat finalRot = worldRot * ToX;

            PxCapsuleGeometry geo(C.Radius, C.Length * 0.5f); // PhysX expects halfLength
            PxTransform pose(ToPxVec3(centerLocal), ToPxQuat(finalRot));
            PxShape* shape = gPhysics->createShape(geo, *gMaterial);
            shape->setLocalPose(pose);
            shape->setSimulationFilterData(FPhysX::MakeFilterData(
                FPhysX::ECollisionGroup::Environment,
                FPhysX::ECollisionGroup::All));
            Shapes.Add(shape);
        }

        // 3d. Convex
        for (const FKConvexElem& X : BodySetup->AggGeom.ConvexElems)
        {
            PxConvexMeshDesc desc;
            desc.points.count = X.VertexData.Num();
            desc.points.stride = sizeof(PxVec3);
            desc.points.data = X.VertexData.GetData();
            desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

            PxDefaultMemoryOutputStream buf;
            if (!gCooking->cookConvexMesh(desc, buf)) continue;

            PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
            PxConvexMesh* mesh = gPhysics->createConvexMesh(input);

            PxShape* shape = gPhysics->createShape(PxConvexMeshGeometry(mesh), *gMaterial);

            FVector center(0, 0, 0);
            for (auto& v : X.VertexData) center += v;
            center /= X.VertexData.Num();

            shape->setLocalPose(PxTransform(ToPxVec3(center)));
            shape->setSimulationFilterData(FPhysX::MakeFilterData(
                FPhysX::ECollisionGroup::Environment,
                FPhysX::ECollisionGroup::All));
            Shapes.Add(shape);
        }

        // 4. 바디 생성 및 씬 등록
        FBodyInstance* Instance = new FBodyInstance(this, EBodyType::Dynamic, ToPxVec3(globalPos), BodySetup->BoneName);

        for (PxShape* shape : Shapes)
        {
            Instance->AttachShape(*shape);
        }
        
        PxRigidBodyExt::updateMassAndInertia(*Instance->RigidDynamicHandle, 1.0f);
        Instance->RigidActorHandle->userData = (void*)Instance;
        GetWorld()->GetPhysicsScene()->addActor(*Instance->RigidDynamicHandle);
        Bodies.Add(Instance);
    }
    PhysicsAsset->UpdateBodySetupIndexMap();
}

void USkeletalMeshComponent::InstantiatePhysicsAssetConstraints_Internal()
{
    Constraints.Empty();

     //PhysicsAsset의 ConstraintSetup 배열 순회
     for (const UConstraintSetup* ConstraintSetup : GetSkeletalMesh()->GetPhysicsAsset()->ConstraintSetup)
     {
         // 1. 부모/자식 바디 찾기
         FBodyInstance* parentBody = Bodies[*(GetSkeletalMesh()->GetPhysicsAsset()->BodySetupIndexMap.Find(ConstraintSetup->JointElem.ParentBoneName))];
         FBodyInstance* childBody = Bodies[*(GetSkeletalMesh()->GetPhysicsAsset()->BodySetupIndexMap.Find(ConstraintSetup->JointElem.ChildBoneName))];

         PxVec3 anchorVec = ToPxVec3(ConstraintSetup->JointElem.Center);
         PxQuat anchorRot = ToPxQuat(ConstraintSetup->JointElem.Rotation);

         PxTransform anchorTransform = PxTransform(anchorVec, anchorRot);
         
         // 2. 조인트 로컬 프레임 계산
         PxTransform localParent = PxTransform(parentBody->RigidActorHandle->getGlobalPose().getInverse() * anchorTransform);
         PxTransform localChild  = PxTransform(childBody->RigidActorHandle->getGlobalPose().getInverse() * anchorTransform);
    
         // 3. PhysX 조인트 생성
         PxD6Joint* joint = PxD6JointCreate(*gPhysics, parentBody->RigidActorHandle, localParent, childBody->RigidActorHandle, localChild);
    
         // 4. 조인트 제한/설정 적용
         for (int axis = 0; axis < 6; ++axis)
         {
             joint->setMotion(static_cast<PxD6Axis::Enum>(axis), static_cast<PxD6Motion::Enum>(ConstraintSetup->JointElem.AxisMotions[axis]));
         }
         joint->setTwistLimit(PxJointAngularLimitPair(
             ConstraintSetup->JointElem.TwistLimitMin, ConstraintSetup->JointElem.TwistLimitMax));
         joint->setSwingLimit(PxJointLimitCone(
             ConstraintSetup->JointElem.SwingLimitMax1, ConstraintSetup->JointElem.SwingLimitMax2));
    
         // 5. ConstraintInstance에 저장
         FConstraintInstance* Instance = new FConstraintInstance(this, ConstraintSetup->JointName, joint);
         Instance->JointHandle->userData = (void*)Instance;

         Constraints.Add(Instance);
     }
}
