struct VS_OUTPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float2 uv : TEXCOORD0; // UV 좌표
};

// SV_VertexID 로 0,1,2 세 정점을 풀스크린 삼각형으로 배치
VS_OUTPUT mainVS(uint VertexID : SV_VertexID)
{
    VS_OUTPUT OUT;

    // 풀스크린 삼각형 정점 좌표
    float2 pos[3] =
    {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0)
    };

    // NDC 위치
    OUT.position = float4(pos[VertexID], 0.0, 1.0);

    // NDC 를 UV [0,1] 로 매핑
    OUT.uv = pos[VertexID] * 0.5 + 0.5;

    return OUT;
}