#pragma once
#include "FBaseRenderPass.h"

#include <d3d11.h>

class FDepthOfFieldRenderPass : public FBaseRenderPass
{
public:
    explicit FDepthOfFieldRenderPass(const FName& InShaderName);

    virtual ~FDepthOfFieldRenderPass() {}
    void AddRenderObjectsToRenderPass(UWorld* World) override;
    void Prepare(std::shared_ptr<FViewportClient> InViewportClient) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;

    void UpdateDoFConstant(const std::shared_ptr<FViewportClient> InViewportClient) const;

private:
    bool bRender;
    ID3D11Buffer* DoFConstantBuffer = nullptr;
};
