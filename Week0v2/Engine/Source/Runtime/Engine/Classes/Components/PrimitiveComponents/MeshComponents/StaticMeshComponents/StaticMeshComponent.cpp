#include "StaticMeshComponent.h"

#include "Engine/World.h"
#include "Launch/EditorEngine.h"
#include "UObject/ObjectFactory.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Classes/Engine/FLoaderOBJ.h"
#include "Components/Mesh/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "PhysicsEngine/ConvexElem.h"
#include "PhysicsEngine/SphereElem.h"
#include "PhysicsEngine/SphylElem.h"

UStaticMeshComponent::UStaticMeshComponent(const UStaticMeshComponent& Other)
    : UMeshComponent(Other)
    , staticMesh(Other.staticMesh)
    , selectedSubMeshIndex(Other.selectedSubMeshIndex)
{
}
uint32 UStaticMeshComponent::GetNumMaterials() const
{
    if (staticMesh == nullptr) return 0;

    return staticMesh->GetMaterials().Num();
}

UMaterial* UStaticMeshComponent::GetMaterial(uint32 ElementIndex) const
{
    if (staticMesh != nullptr)
    {
        if (OverrideMaterials[ElementIndex] != nullptr)
        {
            return OverrideMaterials[ElementIndex];
        }
    
        if (staticMesh->GetMaterials().IsValidIndex(ElementIndex))
        {
            return staticMesh->GetMaterials()[ElementIndex]->Material;
        }
    }
    return nullptr;
}

uint32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
    if (staticMesh == nullptr) return -1;

    return staticMesh->GetMaterialIndex(MaterialSlotName);
}

TArray<FName> UStaticMeshComponent::GetMaterialSlotNames() const
{
    TArray<FName> MaterialNames;
    if (staticMesh == nullptr) return MaterialNames;

    for (const FMaterialSlot* Material : staticMesh->GetMaterials())
    {
        MaterialNames.Emplace(Material->MaterialSlotName);
    }

    return MaterialNames;
}

void UStaticMeshComponent::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    if (staticMesh == nullptr) return;
    staticMesh->GetUsedMaterials(Out);
    for (int materialIndex = 0; materialIndex < GetNumMaterials(); materialIndex++)
    {
        if (OverrideMaterials[materialIndex] != nullptr)
        {
            Out[materialIndex] = OverrideMaterials[materialIndex];
        }
    }
}

int UStaticMeshComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    if (!AABB.IntersectRay(rayOrigin, rayDirection, pfNearHitDistance)) return 0;
    int nIntersections = 0;
    if (staticMesh == nullptr) return 0;

    OBJ::FStaticMeshRenderData* renderData = staticMesh->GetRenderData();

    FVertexSimple* vertices = renderData->Vertices.GetData();
    int vCount = renderData->Vertices.Num();
    UINT* indices = renderData->Indices.GetData();
    int iCount = renderData->Indices.Num();

    if (!vertices) return 0;
    BYTE* pbPositions = reinterpret_cast<BYTE*>(renderData->Vertices.GetData());

    int nPrimitives = (!indices) ? (vCount / 3) : (iCount / 3);
    float fNearHitDistance = FLT_MAX;
    for (int i = 0; i < nPrimitives; i++) {
        int idx0, idx1, idx2;
        if (!indices) {
            idx0 = i * 3;
            idx1 = i * 3 + 1;
            idx2 = i * 3 + 2;
        }
        else {
            idx0 = indices[i * 3];
            idx2 = indices[i * 3 + 1];
            idx1 = indices[i * 3 + 2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        uint32 stride = sizeof(FVertexSimple);
        FVector v0 = *reinterpret_cast<FVector*>(pbPositions + idx0 * stride);
        FVector v1 = *reinterpret_cast<FVector*>(pbPositions + idx1 * stride);
        FVector v2 = *reinterpret_cast<FVector*>(pbPositions + idx2 * stride);

        float fHitDistance;
        if (IntersectRayTriangle(rayOrigin, rayDirection, v0, v1, v2, fHitDistance)) {
            if (fHitDistance < fNearHitDistance) {
                pfNearHitDistance = fNearHitDistance = fHitDistance;
            }
            nIntersections++;
        }

    }
    return nIntersections;
}


void UStaticMeshComponent::SetStaticMesh(UStaticMesh* value)
{ 
    staticMesh = value;
    OverrideMaterials.SetNum(value->GetMaterials().Num());
    AABB = FBoundingBox(staticMesh->GetRenderData()->BoundingBoxMin, staticMesh->GetRenderData()->BoundingBoxMax);
    VBIBTopologyMappingName = staticMesh->GetRenderData()->DisplayName;

    if (GetWorld() == nullptr) return;
    
    UBodySetup* BodySetup = GetStaticMesh()->GetBodySetup();
    
    // 2. 콜라이더 생성
    TArray<PxShape*> shapes;
    // Sphere
    for (const FKSphereElem& sphere : BodySetup->AggGeom.SphereElems)
    {
        PxShape* shape = gPhysics->createShape(PxSphereGeometry(sphere.Radius), *gMaterial);
        // 콜라이더의 로컬 트랜스폼 적용(필요시)
        shape->setLocalPose(PxTransform(ToPxVec3(sphere.Center)));
        PxFilterData fd = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Environment, FPhysX::ECollisionGroup::All);
        shape->setSimulationFilterData(fd);
        shapes.Add(shape);
    }
    // Box
    for (const FKBoxElem& box : BodySetup->AggGeom.BoxElems)
    {
        PxShape* shape = gPhysics->createShape(PxBoxGeometry(box.X/2, box.Y/2, box.Z/2), *gMaterial);
        shape->setLocalPose(PxTransform(ToPxVec3(box.Center), ToPxQuat(box.Rotation.ToQuaternion())));
        PxFilterData fd = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Environment, FPhysX::ECollisionGroup::All);
        shape->setSimulationFilterData(fd);
        shapes.Add(shape);
    }
    // Capsule
    for (const FKSphylElem& capsule : BodySetup->AggGeom.SphylElems)
    {
        PxShape* shape = gPhysics->createShape(PxCapsuleGeometry(capsule.Radius, capsule.Length/2), *gMaterial);
        shape->setLocalPose(PxTransform(ToPxVec3(capsule.Center), ToPxQuat(capsule.Rotation.ToQuaternion())));
        PxFilterData fd = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Environment, FPhysX::ECollisionGroup::All);
        shape->setSimulationFilterData(fd);
        shapes.Add(shape);
    }
    // Convex
    for (const FKConvexElem& convex : BodySetup->AggGeom.ConvexElems)
    {
        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = convex.VertexData.Num();
        convexDesc.points.stride = sizeof(PxVec3);
        convexDesc.points.data = convex.VertexData.GetData();
        convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
        
        PxDefaultMemoryOutputStream buf;
        
        if (!gCooking->cookConvexMesh(convexDesc, buf))
            continue;

        PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
        PxConvexMesh* convexMesh = gPhysics->createConvexMesh(input);
        
        PxShape* shape = gPhysics->createShape(PxConvexMeshGeometry(convexMesh), *gMaterial);
        
        FVector center(0,0,0);
        for (const FVector& v : convex.VertexData)
            center += v;
        center /= convex.VertexData.Num();

        shape->setLocalPose(PxTransform(ToPxVec3(center)));
        PxFilterData fd = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Environment, FPhysX::ECollisionGroup::All);
        shape->setSimulationFilterData(fd);
        shapes.Add(shape);
    }
    
    FBodyInstance* Instance = new FBodyInstance(this, EBodyType::Dynamic, ToPxVec3(GetRelativeLocation()), FName());
    for (PxShape* shape : shapes)
    {
        Instance->AttachShape(*shape);
    }
    Instance->RigidActorHandle->userData = (void*)Instance;
    GetWorld()->GetPhysicsScene()->addActor(*Instance->RigidDynamicHandle);

    BodyInstance = *Instance;
}

std::unique_ptr<FActorComponentInfo> UStaticMeshComponent::GetComponentInfo()
{
    auto Info = std::make_unique<FStaticMeshComponentInfo>();
    SaveComponentInfo(*Info);
    
    return Info;
}

void UStaticMeshComponent::SaveComponentInfo(FActorComponentInfo& OutInfo)
{
    FStaticMeshComponentInfo* Info = static_cast<FStaticMeshComponentInfo*>(&OutInfo);
    Super::SaveComponentInfo(*Info);

    Info->StaticMeshPath = staticMesh->GetRenderData()->PathName;
}
void UStaticMeshComponent::LoadAndConstruct(const FActorComponentInfo& Info)
{
    Super::LoadAndConstruct(Info);

    const FStaticMeshComponentInfo& StaticMeshInfo = static_cast<const FStaticMeshComponentInfo&>(Info);
    UStaticMesh* Mesh = FManagerOBJ::CreateStaticMesh(FString::ToFString(StaticMeshInfo.StaticMeshPath));
    SetStaticMesh(Mesh);

}
UObject* UStaticMeshComponent::Duplicate(UObject* InOuter)
{
    UStaticMeshComponent* NewComp = FObjectFactory::ConstructObjectFrom<UStaticMeshComponent>(this, InOuter);
    NewComp->DuplicateSubObjects(this, InOuter);
    NewComp->PostDuplicate();
    return NewComp;
}

void UStaticMeshComponent::DuplicateSubObjects(const UObject* Source, UObject* InOuter)
{
    UMeshComponent::DuplicateSubObjects(Source, InOuter);
}

void UStaticMeshComponent::PostDuplicate() {}

void UStaticMeshComponent::TickComponent(float DeltaTime)
{
    //Timer += DeltaTime * 0.005f;
    //SetLocation(GetWorldLocation()+ (FVector(1.0f,1.0f, 1.0f) * sin(Timer)));
}
