#include "BlurRenderPass.h"

#include "FadeRenderPass.h"

#include "EditorEngine.h"
#include "LaunchEngineLoop.h"
#include "ShowFlags.h"
#include "Viewport.h"
#include "D3D11RHI/CBStructDefine.h"
#include "D3D11RHI/GraphicDevice.h"
#include "LevelEditor/SLevelEditor.h"
#include "Renderer/Renderer.h"
#include "SlateCore/Layout/SlateRect.h"
#include "UnrealEd/EditorViewportClient.h"

class FRenderResourceManager;

FBlurRenderPass::FBlurRenderPass(const FName& InShaderName)
    : FBaseRenderPass(InShaderName)
{
    FRenderer& Renderer = GEngineLoop.Renderer;
    FRenderResourceManager* RenderResourceManager = Renderer.GetResourceManager();
    bRender = true;
    BlurConstantBuffer = RenderResourceManager->CreateConstantBuffer(sizeof(FFadeConstants));

    FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(FDoFConstants);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;
    HRESULT hr = Graphics.Device->CreateBuffer(&cbDesc, nullptr, &DoFConstantBuffer);
    if (FAILED(hr))
    {
        // 에러 처리
    }
}

void FBlurRenderPass::AddRenderObjectsToRenderPass(UWorld* World)
{
}

void FBlurRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    bRender = true;
    if (bRender)
    {
        FBaseRenderPass::Prepare(InViewportClient);
        const FRenderer& Renderer = GEngineLoop.Renderer;
        FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
        Graphics.SwapPingPongBuffers();

        const auto CurRTV = Graphics.GetCurrentRenderTargetView();
        Graphics.DeviceContext->OMSetRenderTargets(1, &CurRTV, nullptr);
        Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::DepthNone), 0);

        Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        ID3D11SamplerState* Sampler = Renderer.GetSamplerState(ESamplerType::Point);
        Graphics.DeviceContext->PSSetSamplers(0, 1, &Sampler);

        const auto PreviousSRV = Graphics.GetPreviousShaderResourceView();
        Graphics.DeviceContext->PSSetShaderResources(0, 1, &PreviousSRV);
        Graphics.DeviceContext->PSSetShaderResources(1, 1, &Graphics.GetCurrentWindowData()->DepthCopySRV);
    }
}

void FBlurRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    if ( !(std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient)->GetShowFlag() & EEngineShowFlags::SF_DOF))
    {
        UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
        EditorEngine->testBlurStrength =0.0f;
    }
    else
    {
        UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
        EditorEngine->testBlurStrength =1.0f;
    }
    FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;

    if (bRender)
    {
        auto viewPort = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);
        if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            UpdateScreenConstant(InViewportClient);
            UpdateDOFConstant(InViewportClient);
            UpdateBlurConstant(EditorEngine->testBlurStrength,1 / viewPort->GetViewport()->GetFSlateRect().Width, 1 / viewPort->GetViewport()->GetFSlateRect().Height);
        }
        Graphics.DeviceContext->Draw(6, 0);

        bRender = false;
    }
}

void FBlurRenderPass::UpdateBlurConstant(float BlurStrength, float TexelX, float TexelY) const
{
    const FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    FRenderResourceManager* renderResourceManager = GEngineLoop.Renderer.GetResourceManager();

    FBlurConstants BlurConstants;
    BlurConstants.BlurStrength = BlurStrength;
    BlurConstants.TexelSizeX = TexelX;
    BlurConstants.TexelSizeY = TexelY;
    
    renderResourceManager->UpdateConstantBuffer(BlurConstantBuffer, &BlurConstants);
    Graphics.DeviceContext->PSSetConstantBuffers(0, 1, &BlurConstantBuffer);
}

void FBlurRenderPass::UpdateDOFConstant(std::shared_ptr<FViewportClient> InViewportClient) const
{
    const FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    FRenderResourceManager* renderResourceManager = GEngineLoop.Renderer.GetResourceManager();
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);

    FDoFConstants DofConstant;

    // 테스트용 기본 값 설정
    DofConstant.NearPlane = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient)->GetNearClip();
    DofConstant.FarPlane = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient)->GetFarClip();
    DofConstant.FocusDepth   = GEngineLoop.FocusDepth;   // 초점 맞출 거리 (world space depth)
    DofConstant.FocusRange   = GEngineLoop.FocusRange;    // 초점 허용 범위
    DofConstant.MaxCoc       = GEngineLoop.MaxCoc;      // 최대 Circle of Confusion 크기 (픽셀 단위)

    FVector2D viewportSize;
    viewportSize.X =curEditorViewportClient->GetD3DViewport().Width;
    viewportSize.Y =curEditorViewportClient->GetD3DViewport().Height;
    DofConstant.InvScreenSize   = FVector2D(1.0f / viewportSize.X, 1.0f / viewportSize.Y);

    // 업데이트
    renderResourceManager->UpdateConstantBuffer(DoFConstantBuffer, &DofConstant);

    // 픽셀 셰이더의 b2 슬롯에 바인딩
    Graphics.DeviceContext->PSSetConstantBuffers(2, 1, &DoFConstantBuffer);
}
