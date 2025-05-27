#include "DepthOfFieldRenderPass.h"
#include <D3D11RHI/CBStructDefine.h>

#include "LaunchEngineLoop.h"
#include "Renderer/Renderer.h"
#include "UnrealEd/EditorViewportClient.h"


FDepthOfFieldRenderPass::FDepthOfFieldRenderPass(const FName& InShaderName)
:FBaseRenderPass(InShaderName)
{
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
    bRender = true;
}

void FDepthOfFieldRenderPass::AddRenderObjectsToRenderPass(UWorld* World)
{

}

void FDepthOfFieldRenderPass::Prepare(std::shared_ptr<FViewportClient> InViewportClient)
{
    bRender = true;
    if (bRender)
    {
        FBaseRenderPass::Prepare(InViewportClient);
        const FRenderer& Renderer = GEngineLoop.Renderer;
        FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;

        const auto CurRTV = Graphics.GetCurrentRenderTargetView();
        Graphics.DeviceContext->OMSetRenderTargets(1, &CurRTV, nullptr);
        Graphics.DeviceContext->CopyResource(Graphics.GetCurrentWindowData()->DepthCopyTexture, Graphics.GetCurrentWindowData()->DepthStencilBuffer);
        Graphics.DeviceContext->OMSetDepthStencilState(Renderer.GetDepthStencilState(EDepthStencilState::DepthNone), 0);

        Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        ID3D11SamplerState* Sampler = Renderer.GetSamplerState(ESamplerType::Linear);
        Graphics.DeviceContext->PSSetSamplers(0, 1, &Sampler);
        Graphics.DeviceContext->PSSetSamplers(1, 1, &Sampler);
        
        const auto PreviousSRV = Graphics.GetPreviousShaderResourceView();
        Graphics.DeviceContext->PSSetShaderResources(0, 1, &PreviousSRV);
        Graphics.DeviceContext->PSSetShaderResources(1, 1, &Graphics.GetCurrentWindowData()->DepthCopySRV);
    }
}

void FDepthOfFieldRenderPass::Execute(std::shared_ptr<FViewportClient> InViewportClient)
{
    if (!bRender) return;
    
    FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;

    // 1) IA 단계: 입력 레이아웃만 세팅하고, VB/IB 언바인딩
    Graphics.DeviceContext->IASetInputLayout(nullptr);
    Graphics.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 2) 셰이더는 Prepare() 단계에서 이미 바인딩됨(VS, PS, CB, SRV, Sampler)
    UpdateDoFConstant(InViewportClient);
    // 3) 풀스크린 삼각형 드로우: 3개의 정점으로 화면 전체 커버
    Graphics.DeviceContext->Draw(3, 0);

    // 4) 한 번만 그리도록 플래그 리셋
    bRender = false;
}

void FDepthOfFieldRenderPass::UpdateDoFConstant(const std::shared_ptr<FViewportClient> InViewportClient) const
{
    const FGraphicsDevice& Graphics = GEngineLoop.GraphicDevice;
    FRenderResourceManager* renderResourceManager = GEngineLoop.Renderer.GetResourceManager();
    std::shared_ptr<FEditorViewportClient> curEditorViewportClient = std::dynamic_pointer_cast<FEditorViewportClient>(InViewportClient);

    FDoFConstants DofConstant;

    // 테스트용 기본 값 설정
    DofConstant.FocusDepth   = 1000.0f;   // 초점 맞출 거리 (world space depth)
    DofConstant.FocusRange   = 300.0f;    // 초점 허용 범위
    DofConstant.MaxCoc       = 8.0f;      // 최대 Circle of Confusion 크기 (픽셀 단위)

    FVector2D viewportSize;
    viewportSize.X =curEditorViewportClient->GetD3DViewport().Width;
    viewportSize.Y =curEditorViewportClient->GetD3DViewport().Height;
    DofConstant.InvScreenSize   = FVector2D(1.0f / viewportSize.X, 1.0f / viewportSize.Y);

    // 업데이트
    renderResourceManager->UpdateConstantBuffer(DoFConstantBuffer, &DofConstant);

    // 픽셀 셰이더의 b2 슬롯에 바인딩
    Graphics.DeviceContext->PSSetConstantBuffers(2, 1, &DoFConstantBuffer);
}
