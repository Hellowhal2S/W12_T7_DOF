








#pragma once
#include "FBaseRenderPass.h"
#include "Define.h"
class USkeletalMeshComponent;
class USkeletalMesh;
class UPhysicsAsset;
class UShapeComponent;
class UCapsuleShapeComponent;
class FLineBatchRenderPass : public FBaseRenderPass
{
public:
    explicit FLineBatchRenderPass(const FName& InShaderName);

    void AddRenderObjectsToRenderPass(UWorld* World) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;

    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
    void DrawDebugPhysics(USkeletalMeshComponent* SkeletalMesh);
private:
    static void UpdateBatchResources();
    TArray<UShapeComponent*> ShapeComponents;
    TArray<UCapsuleShapeComponent*> CapsuleShapeComponents;
    FName VBIBTopologyMappingName;
};
