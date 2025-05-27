// DoF_AllInOne.hlsl
#define MAX_RADIUS 5

cbuffer FDoFConstants : register(b0)
{
    float FocusDepth;
    float FocusRange;
    float MaxCoC;
    float Padding0;
    
    float2 InvScreenSize;
    float2 Padding1;
};

Texture2D SceneColor : register(t0);
Texture2D SceneDepth : register(t1);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);

struct VS_OUTPUT
{
    float4 Pos : SV_Position;
    float2 UV : TEXCOORD0;
};

float4 mainPS(VS_OUTPUT IN) : SV_Target
{
    float2 uv = IN.UV;

    // 1) CoC 계산
    float depth = SceneDepth.Sample(DepthSampler, uv).r;
    float coc = saturate(abs(depth - FocusDepth) / FocusRange);
    coc = min(coc, MaxCoC);

    // 2) 간단 블러 (가변 반경, 가중치 가우시안 근사)
    int radius = int(coc * MAX_RADIUS);

    float3 blurCol = float3(0, 0, 0);
    float totalW = 0;

    for (int y = -MAX_RADIUS; y <= MAX_RADIUS; ++y)
    {
        for (int x = -MAX_RADIUS; x <= MAX_RADIUS; ++x)
        {
            if (abs(x) > radius || abs(y) > radius)
                continue;

            float2 offs = float2(x, y) * InvScreenSize;
            float w = 1.0 / (1.0 + (x*x + y*y));
            blurCol += SceneColor.Sample(ColorSampler, IN.UV + offs).rgb * w;
            totalW  += w;
        }
    }

    blurCol /= totalW;

    // 3) 원본 + 블러 합성
    float3 sharp = SceneColor.Sample(ColorSampler, uv).rgb;
    float3 outC = lerp(sharp, blurCol, coc);

    return float4(outC, 1);
}