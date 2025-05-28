#include "LineBatchRenderPass.h"

#include "Define.h"
#include "EditorEngine.h"
#include "Renderer/VBIBTopologyMapping.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "Engine/World.h"

#include "LaunchEngineLoop.h"
#include "ShowFlags.h"
#include "Actors/Light.h"
#include "Actors/SkeletalMeshActor.h"
#include "Components/LightComponents/DirectionalLightComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponents/Physics/UShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UCapsuleShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UBoxShapeComponent.h"
#include "Engine/AssetManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SphereElem.h"
#include "PhysicsEngine/SphylElem.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderResourceManager.h"
#include "UObject/UObjectIterator.h"

class USpotLightComponent;
extern UEngine* GEngine;

FLineBatchRenderPass::FLineBatchRenderPass(const FName& InShaderName) : FBaseRenderPass(InShaderName)
{
    FSimpleVertex vertices[2]{ {0}, {0} };

    FRenderResourceManager* renderResourceManager = GEngineLoop.Renderer.GetResourceManager();
    ID3D11Buffer* pVertexBuffer = renderResourceManager->CreateStaticVertexBuffer<FSimpleVertex>(vertices, 2);
    renderResourceManager->AddOrSetVertexBuffer(TEXT("LineBatchVB"), pVertexBuffer);

    GEngineLoop.Renderer.MappingVBTopology(TEXT("LineBatch"), TEXT("LineBatchVB"), sizeof(FSimpleVertex), 2, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    VBIBTopologyMappingName = TEXT("LineBatch");
}

void FLineBatchRenderPass::AddRenderObjectsToRenderPass(UWorld* World)
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (EditorEngine == nullptr)
    {
        return;
    }
    
    std::shared_ptr<FViewportClient> ViewportClient = EditorEngine->GetLevelEditor()->GetActiveViewportClient();
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(ViewportClient);
    if (!(curEditorViewportClient->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::Type::SF_AABB)))
    {
        return;
    }

    UPrimitiveBatch& PrimitiveBatch = UPrimitiveBatch::GetInstance();
    if (World->WorldType == EWorldType::EditorPhysicsPreview)
    {
        for (auto Actor : World->GetActors())
        {
            if (Cast<ASkeletalMeshActor>(Actor))
            {
                DrawDebugPhysics(Cast<ASkeletalMeshActor>(Actor)->GetSkeletalMeshComponent());
            }
        }
    }
    
    for (UShapeComponent* ShapeComponent : TObjectRange<UShapeComponent>())
    {
        if (ShapeComponent->GetWorld() != World)
        {
            continue;
        }

        const FShapeInfo* BaseShapeInfo = ShapeComponent->GetShapeInfo();

        if (BaseShapeInfo == nullptr)
        {
            continue;
        }
        
        const FBoundingBox& Box = ShapeComponent->GetBroadAABB();
        FVector Center = ShapeComponent->GetWorldLocation();

        PrimitiveBatch.AddAABB(Box, Center, FMatrix::Identity);

        switch (BaseShapeInfo->Type)
        {
        case EShapeType::Box:
            {
                const FBoxShapeInfo* BoxInfo = static_cast<const FBoxShapeInfo*>(BaseShapeInfo);

                FVector BoxExtent = BoxInfo->Extent;
                FVector Center = BoxInfo->Center;
                FMatrix WorldMatrix = BoxInfo->WorldMatrix;

                FBoundingBox localOBB;
                localOBB.min = FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);
                localOBB.max = FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);

                PrimitiveBatch.AddOBB(localOBB, Center, WorldMatrix);
                break;
            }
        case EShapeType::Sphere:
            {
                const FSphereShapeInfo* SphereInfo = static_cast<const FSphereShapeInfo*>(BaseShapeInfo);

                FVector Center = SphereInfo->Center;
                float Radius = SphereInfo->Radius;
                FVector4 Color = FVector4(0.0f, 1.0f, 0.0f, 1.0f); // 사용 안됨.

                PrimitiveBatch.AddSphere(Center, Radius, Color);
                break;
            }
        case EShapeType::Capsule:
            {
                const FCapsuleShapeInfo* CapsuleInfo = static_cast<const FCapsuleShapeInfo*>(BaseShapeInfo);

                FVector Center = CapsuleInfo->Center;
                FVector UpV = CapsuleInfo->Up;
                float Radius = CapsuleInfo->Radius;
                float HalfHeight = CapsuleInfo->HalfHeight;
                FVector4 Color = FVector4(0.0f, 1.0f, 0.0f, 1.0f); // 사용 안됨.

                UpV.Normalize();

                PrimitiveBatch.AddCapsule(Center, UpV, HalfHeight, Radius, Color);
                break;
            }
        default:
            break;
        }
    }
}

void FLineBatchRenderPass::Execute(const std::shared_ptr<FViewportClient> InViewportClient)
{
    FRenderer& Renderer = GEngineLoop.Renderer;
    FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();
    UPrimitiveBatch& PrimitveBatch = UPrimitiveBatch::GetInstance();

    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);

    // 쉐이더 내에서 한 번만 Update되어야하는 CB
    const FMatrix Model = FMatrix::Identity;

    FMatrixBuffer MVPConstant;
    MVPConstant.Model = Model;
    if (curEditorViewportClient != nullptr)
    {
        MVPConstant.ViewProj = curEditorViewportClient->GetViewMatrix() * curEditorViewportClient->GetProjectionMatrix();
    }

    renderResourceManager->UpdateConstantBuffer(TEXT("FMatrixBuffer"), &MVPConstant);
    UpdateScreenConstant(InViewportClient);

    const FGridParametersData GridParameters = PrimitveBatch.GetGridParameters();
    renderResourceManager->UpdateConstantBuffer(TEXT("FGridParametersData"), &GridParameters);

    UpdateBatchResources();

    FPrimitiveCounts PrimitiveCounts;
    PrimitiveCounts.ConeCount = PrimitveBatch.GetCones().Num();
    PrimitiveCounts.BoundingBoxCount = PrimitveBatch.GetBoundingBoxes().Num();
    PrimitiveCounts.SphereCount = PrimitveBatch.GetSpheres().Num();
    PrimitiveCounts.LineCount = PrimitveBatch.GetLines().Num();
    PrimitiveCounts.CapsuleCount = PrimitveBatch.GetCapsules().Num();
    PrimitiveCounts.OBBCount = PrimitveBatch.GetOrientedBoundingBoxes().Num();
    renderResourceManager->UpdateConstantBuffer(TEXT("FPrimitiveCounts"), &PrimitiveCounts);

    const std::shared_ptr<FVBIBTopologyMapping> VBIBTopologyMappingInfo = Renderer.GetVBIBTopologyMapping(VBIBTopologyMappingName);
    VBIBTopologyMappingInfo->Bind();

    const uint32 vertexCountPerInstance = 2;

    const uint32 NumSegmentsCircle = 16;
    const uint32 NumVerticalLines = 4;
    const uint32 NumHemisphereSegments = 8;

    const uint32 capsuleInstancePerCapsule =
        (NumSegmentsCircle * 2) + // Top/Bottom Circle
        NumVerticalLines +        // 세로선 4개
        (NumHemisphereSegments * 4) + // Top 반구 (0°/180°, 90°/270°)
        (NumHemisphereSegments * 4);  // Bottom 반구 (0°/180°, 90°/270°)

    const uint32 instanceCount =
        GridParameters.GridCount +
        3 +
        (PrimitveBatch.GetBoundingBoxes().Num() * 12) +
        (PrimitveBatch.GetCones().Num() * (2 * PrimitveBatch.GetConeSegmentCount())) +
        (PrimitveBatch.GetSpheres().Num() * 3 * 32) +
        (PrimitveBatch.GetLines().Num() * 2) +
        (12 * PrimitveBatch.GetOrientedBoundingBoxes().Num()) +
        (PrimitveBatch.GetCapsules().Num() * capsuleInstancePerCapsule);

    Graphics.DeviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, 0, 0);

    PrimitveBatch.ClearBatchPrimitives();
}

void FLineBatchRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    FBaseRenderPass::Prepare(InViewportClient);

    FRenderer& Renderer = GEngineLoop.Renderer;
    const FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    Graphics.DeviceContext->RSSetState(Renderer.GetCurrentRasterizerState()); //레스터 라이저 상태 설정
    Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::LessEqual), 0);
    
    const auto CurRTV = Graphics.GetCurrentRenderTargetView();
    Graphics.DeviceContext->OMSetRenderTargets(1, &CurRTV, Graphics.GetCurrentWindowData()->DepthStencilView); // 렌더 타겟 설정

    for (AActor* actor : InViewportClient->GetWorld()->GetSelectedActors())
    {
        ALight* Light = Cast<ALight>(actor);
        if (Light)
        {
            TArray<UActorComponent*> Comps = Light->GetComponents();
            for (const auto& Comp : Comps)
            {
                if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Comp))
                {
                    const FMatrix Model = SpotLight->GetWorldMatrix();
                    if (SpotLight->GetOuterConeAngle() > 0)
                    {
                        UPrimitiveBatch::GetInstance().AddCone(
                            SpotLight->GetWorldLocation(),
                            tan(SpotLight->GetOuterConeAngle()) * 15.0f,
                            SpotLight->GetWorldLocation() + SpotLight->GetWorldForwardVector() * 15.0f,
                            15,
                            SpotLight->GetLightColor()
                        );
                    }
                    if (SpotLight->GetInnerConeAngle() > 0)
                    {
                        UPrimitiveBatch::GetInstance().AddCone(
                            SpotLight->GetWorldLocation(),
                            tan(SpotLight->GetInnerConeAngle()) * 15.0f,
                            SpotLight->GetWorldLocation() + SpotLight->GetWorldForwardVector() * 15.0f,
                            15,
                            SpotLight->GetLightColor()
                        );
                    }
                }
                else if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(Comp))
                {
                    FVector Right = DirectionalLight->GetWorldRightVector();
                    for (int i = 0; i < 4; ++i)
                    {
                        UPrimitiveBatch::GetInstance().AddLine(
                            DirectionalLight->GetWorldLocation() + Right * (-1.5f + i),
                            DirectionalLight->GetWorldForwardVector(),
                            15.0f,
                            DirectionalLight->GetLightColor()
                        );
                    }
                }
                if (UPointLightComponent* PointLight = Cast< UPointLightComponent>(Comp))
                {
                    if (PointLight->GetRadius() > 0)
                    {
                        UPrimitiveBatch::GetInstance().AddSphere(
                            PointLight->GetWorldLocation(),
                            PointLight->GetRadius(),
                            PointLight->GetLightColor()
                        );
                    }
                }
            }
        }
    }
    ID3D11ShaderResourceView* FBoundingBoxSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("BoundingBox"));
    Graphics.DeviceContext->VSSetShaderResources(3, 1, &FBoundingBoxSRV);
    ID3D11ShaderResourceView* FConeSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Cone"));
    Graphics.DeviceContext->VSSetShaderResources(4, 1, &FConeSRV);
    ID3D11ShaderResourceView* FOBBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("OBB"));
    Graphics.DeviceContext->VSSetShaderResources(5, 1, &FOBBSRV);
    ID3D11ShaderResourceView* FSphereSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Sphere"));
    Graphics.DeviceContext->VSSetShaderResources(6, 1, &FSphereSRV);
    ID3D11ShaderResourceView* FLineSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Line"));
    Graphics.DeviceContext->VSSetShaderResources(7, 1, &FLineSRV);
    ID3D11ShaderResourceView* FCapsuleSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Capsule"));
    Graphics.DeviceContext->VSSetShaderResources(8, 1, &FCapsuleSRV);
}

void FLineBatchRenderPass::ClearRenderObjects()
{
    ShapeComponents.Empty();
    CapsuleShapeComponents.Empty();
}

void FLineBatchRenderPass::DrawDebugPhysics(USkeletalMeshComponent* SkeletalMesh)
{
            // 싱글톤 얻어오기
    UPrimitiveBatch& PB = UPrimitiveBatch::GetInstance();
    
    const FVector4 ColorSphere  = FVector4(0.6f, 0.2f, 0.8f, 1.0f);
    const FVector4 ColorBox     = FVector4(0.6f, 0.2f, 0.8f, 1.0f);
    const FVector4 ColorCapsule = FVector4(0.6f, 0.2f, 0.8f, 1.0f);
    
    // --- UBodySetup 기반 시각화 예시 ---
    if (UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetSkeletalMesh()->GetPhysicsAsset())
    {
        for (UBodySetup* BS : PhysicsAsset->BodySetup)
        {
            if (!BS) continue;

            FTransform boneXF;
            FMatrix boneM;
            // 본에 매핑된 월드 트랜스폼(본 월드 매트릭스) 얻어오기
            for (auto Bone : SkeletalMesh->GetSkeletalMesh()->GetRenderData().Bones)
            {
                if (Bone.BoneName == BS->BoneName.ToString())
                {
                    boneXF.SetLocation(Bone.GlobalTransform.GetTranslationVector());
                    FRotator BoneRot;
                    Bone.GlobalTransform.GetRotationMatrix(BoneRot);
                    boneXF.SetRotation(BoneRot.ToQuaternion());
                    boneXF.SetScale(Bone.GlobalTransform.GetScaleVector());
                    boneM = Bone.GlobalTransform;
                }
            }
           boneM =  boneM*SkeletalMesh->GetWorldMatrix();
            // FMatrix boneM    = boneXF.ToMatrixWithScale();

            // 1) Spheres
            for (const FKSphereElem& S : BS->AggGeom.SphereElems)
            {
                // 로컬 센터를 월드로 변환
                FVector centerWS = boneM.TransformPosition(S.Center);
                PB.AddSphere(centerWS, S.Radius, ColorSphere);
            }

            // 2) Boxes (AABB 로 그리기)
            for (const FKBoxElem& B : BS->AggGeom.BoxElems)
            {
                // BoxElem.Center 는 로컬 오프셋, (X,Y,Z) 는 half‐extents
                FBoundingBox localAABB(
                    FVector(-B.X, -B.Y, -B.Z),
                    FVector( B.X,  B.Y,  B.Z)
                );
                FVector worldCenter = boneM.TransformPosition(B.Center);
                FMatrix LastboneM = boneM*FMatrix::CreateRotationMatrix(B.Rotation.Roll,B.Rotation.Pitch,B.Rotation.Yaw) ;
                PB.AddAABB(localAABB, worldCenter, LastboneM);
            }

            // 3) Capsules
            for (const FKSphylElem& C : BS->AggGeom.SphylElems)
            {
                // 로컬 -> 월드
                FVector centerWS = boneM.TransformPosition(C.Center);
                // 캡슐 축 방향: 로컬 Z 축을 UpVector 로 가정
                FMatrix LastboneM  = boneM*FMatrix::CreateRotationMatrix(C.Rotation.Roll,C.Rotation.Pitch,C.Rotation.Yaw);
                FVector upWS = FMatrix::TransformVector(FVector::UpVector,LastboneM);
                PB.AddCapsule(centerWS, upWS, C.Length * 0.5f, C.Radius, ColorCapsule);
            }

            // 4) Convex (간단히 OBB 로 감싸기)
            // for (const FKConvexElem& X : BS->AggGeom.ConvexElems)
            // {
            //     // ConvexElem.VertexData 를 최소한 2개 이상 가지고
            //     if (X.VertexData.Num() >= 2)
            //     {
            //         // 로컬 버텍스 중 AABB 계산
            //         FBoundingBox localBB;
            //         for (const FVector& V : X.VertexData)
            //             localBB = localBB +  V;
            //
            //         FVector worldCenter = boneM.TransformPosition(localBB.GetCenter());
            //         PB.AddOBB(localBB, worldCenter, boneM);
            //     }
            // }
        }
    }
}

void FLineBatchRenderPass::UpdateBatchResources()
{
    FRenderer& Renderer = GEngineLoop.Renderer;
    FRenderResourceManager* renderResourceManager = Renderer.GetResourceManager();

    UPrimitiveBatch& PrimitveBatch = UPrimitiveBatch::GetInstance();

    {
        if (PrimitveBatch.GetBoundingBoxes().Num() > PrimitveBatch.GetAllocatedBoundingBoxCapacity())
        {
            PrimitveBatch.SetAllocatedBoundingBoxCapacity(PrimitveBatch.GetBoundingBoxes().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FBoundingBox>(static_cast<uint32>(PrimitveBatch.GetAllocatedBoundingBoxCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(PrimitveBatch.GetAllocatedBoundingBoxCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("BoundingBox"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("BoundingBox"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("BoundingBox"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("BoundingBox"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, PrimitveBatch.GetBoundingBoxes());
        }
    }

    {
        if (UPrimitiveBatch::GetInstance().GetCones().Num() > UPrimitiveBatch::GetInstance().GetAllocatedConeCapacity())
        {
            UPrimitiveBatch::GetInstance().SetAllocatedConeCapacity(UPrimitiveBatch::GetInstance().GetCones().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FCone>(static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedConeCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedConeCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Cone"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Cone"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Cone"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Cone"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, UPrimitiveBatch::GetInstance().GetCones());
        }
    }

    {
        if (UPrimitiveBatch::GetInstance().GetSpheres().Num() > UPrimitiveBatch::GetInstance().GetAllocatedSphereCapacity())
        {
            UPrimitiveBatch::GetInstance().SetAllocatedSphereCapacity(UPrimitiveBatch::GetInstance().GetSpheres().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FSphere>(static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedSphereCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedSphereCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Sphere"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Sphere"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Sphere"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Sphere"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, UPrimitiveBatch::GetInstance().GetSpheres());
        }
    }

    {
        if (PrimitveBatch.GetLines().Num() > PrimitveBatch.GetAllocatedLineCapacity())
        {
            PrimitveBatch.SetAllocatedLineCapacity(PrimitveBatch.GetLines().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FLine>(static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedLineCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(UPrimitiveBatch::GetInstance().GetAllocatedLineCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Line"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Line"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Line"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Line"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, UPrimitiveBatch::GetInstance().GetLines());
        }
    }
    {
        int32 CapsulesNum = PrimitveBatch.GetCapsules().Num();
        if (CapsulesNum > PrimitveBatch.GetAllocatedCapsuleCapacity())
        {
            PrimitveBatch.SetAllocatedCapsuleCapacity(CapsulesNum);

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FCapsule>(static_cast<uint32>(PrimitveBatch.GetAllocatedCapsuleCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(PrimitveBatch.GetAllocatedCapsuleCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("Capsule"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("Capsule"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("Capsule"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("Capsule"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, PrimitveBatch.GetCapsules());
        }
    }

    {
        if (PrimitveBatch.GetOrientedBoundingBoxes().Num() > PrimitveBatch.GetAllocatedOBBCapacity())
        {
            PrimitveBatch.SetAllocatedOBBCapacity(PrimitveBatch.GetOrientedBoundingBoxes().Num());

            ID3D11Buffer* SB = nullptr;
            ID3D11ShaderResourceView* SBSRV = nullptr;
            SB = renderResourceManager->CreateStructuredBuffer<FOBB>(static_cast<uint32>(PrimitveBatch.GetAllocatedOBBCapacity()));
            SBSRV = renderResourceManager->CreateBufferSRV(SB, static_cast<uint32>(PrimitveBatch.GetAllocatedOBBCapacity()));

            renderResourceManager->AddOrSetSRVStructuredBuffer(TEXT("OBB"), SB);
            renderResourceManager->AddOrSetSRVStructuredBufferSRV(TEXT("OBB"), SBSRV);
        }

        ID3D11Buffer* SB = renderResourceManager->GetSRVStructuredBuffer(TEXT("OBB"));
        ID3D11ShaderResourceView* SBSRV = renderResourceManager->GetStructuredBufferSRV(TEXT("OBB"));
        if (SB != nullptr && SBSRV != nullptr)
        {
            renderResourceManager->UpdateStructuredBuffer(SB, PrimitveBatch.GetOrientedBoundingBoxes());
        }
    }
}
