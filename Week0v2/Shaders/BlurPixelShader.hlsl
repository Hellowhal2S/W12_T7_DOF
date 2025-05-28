#include "ShaderHeaders/GConstantBuffers.hlsli"

cbuffer BlurConstants : register(b0) // 다른 상수 버퍼와 겹치지 않는 슬롯 사용
{
    // 블러 강도 (가우시안 함수의 표준 편차(sigma) 역할)
    float BlurStrength;
    // 텍셀(텍스처 픽셀 하나)의 UV 공간 크기 (x: 가로 크기, y: 세로 크기)
    float2 TexelSize;
    // 필요시 패딩
    float Padding;
};
cbuffer FDoFConstants : register(b2)
{
    float NearPlane;
    float FarPlane;
    float FocusDepth;
    float FocusRange;
    
    float MaxCoC;
    float2 InvScreenSize;
    float Padding0;
};

// 입력 텍스처 (원본 씬 텍스처)
Texture2D InputTexture : register(t0);
Texture2D SceneDepth : register(t1);
SamplerState SamplerLinear : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};
// 가우시안 함수 (상대 가중치 계산용)
// exp(-(x^2) / (2 * sigma^2))
float GaussianWeight(float offset, float sigma)
{
    if (sigma <= 0.001f)
    {
        return (abs(offset) < 0.001f) ? 1.0f : 0.0f;
    }
    float sigmaSq = sigma * sigma;
    return exp(-(offset * offset) / (2.0f * sigmaSq));
}
// --- 단일 패스 가우시안 블러 픽셀 셰이더 ---
// 블러 샘플링 반경 (예: 3이면 -3 ~ +3 범위, 총 7x7 커널)
// **주의: 이 값이 커지면 성능이 급격히 저하됩니다!** (샘플링 횟수 = (2*RADIUS+1)^2)
static const int KERNEL_RADIUS = 11; // 7x7 커널 예시 (49 샘플)
float4 mainPS(VS_OUT input) : SV_TARGET
{
    float z = SceneDepth.Sample(SamplerLinear, input.uv).r;
// 1) 선형화
    float viewZ = (NearPlane * FarPlane) / (FarPlane - z * (FarPlane - NearPlane));
// (선택) 정규화: viewZ = (viewZ - NearPlane) / (FarPlane - NearPlane);

    float distanceFromFocus = abs(viewZ - FocusDepth);

    float coc = 0.0;
    if (distanceFromFocus > FocusRange)
    {
        coc = saturate((distanceFromFocus - FocusRange) / FocusRange);
        coc = min(coc, MaxCoC);
    }
    
    int radius = int(coc * KERNEL_RADIUS);
    
    float2 viewportUV = input.uv * ViewportSize + ViewportOffset;
    float4 totalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    float sigma = BlurStrength; // 가우시안 표준 편차
    // 2D 커널 전체를 순회 (-KERNEL_RADIUS 부터 +KERNEL_RADIUS 까지)
    [unroll] // 바깥 루프 풀기
    for (int j = -KERNEL_RADIUS; j <= KERNEL_RADIUS; ++j)
    {
        [unroll] // 안쪽 루프 풀기
        for (int i = -KERNEL_RADIUS; i <= KERNEL_RADIUS; ++i)
        {
            if (abs(i) <= radius && abs(j) <= radius)
            {
                float2 offsetUV = viewportUV + float2(TexelSize.x * i, TexelSize.y * j);
                float weightX = GaussianWeight(float(i), sigma);
                float weightY = GaussianWeight(float(j), sigma);
                float weight = weightX * weightY;
                totalColor += InputTexture.Sample(SamplerLinear, offsetUV) * weight;
                totalWeight += weight;
            }
        }
    }
    // 최종 색상 계산 (가중 평균)
    if (totalWeight > 0.0f)
    {
        return totalColor / totalWeight;
    }
    else // 블러 강도가 0에 가까워 가중치 합이 0이 되는 경우 등
    {
        // 중앙 픽셀 색상 반환 (블러 없음)
        return InputTexture.Sample(SamplerLinear, viewportUV);
    }
}