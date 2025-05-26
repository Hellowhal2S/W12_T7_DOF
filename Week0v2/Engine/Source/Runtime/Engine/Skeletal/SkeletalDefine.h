#pragma once
#include "Define.h"
#include "LaunchEngineLoop.h"
#include "Container/Array.h"
#include "Container/Map.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Vector.h"

#pragma region Skeletal;
struct FBone
{
    FString BoneName;
    FMatrix SkinningMatrix;
    FMatrix InverseBindPoseMatrix;
    FMatrix GlobalTransform;
    FMatrix LocalTransform;
    int ParentIndex;

    void Serialize(FArchive& Ar) const
    {
        Ar << BoneName
            << SkinningMatrix
            << InverseBindPoseMatrix
            << GlobalTransform
            << LocalTransform
            << ParentIndex;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> BoneName
            >> SkinningMatrix
            >> InverseBindPoseMatrix
            >> GlobalTransform
            >> LocalTransform
            >> ParentIndex;
    }
};

struct FBoneNode
{
    FString BoneName;
    int BoneIndex;             // Index in the Bones array
    TArray<int> ChildIndices;  // Indices of child bones

    void Serialize(FArchive& Ar) const
    {
        Ar << BoneName << BoneIndex << ChildIndices;
    }
    
    void Deserialize(FArchive& Ar)
    {
        Ar >> BoneName >> BoneIndex >> ChildIndices;
    }
};

struct FSkeletalVertex
{
    FVector4 Position;
    FVector Normal;
    FVector4 Tangent;
    FVector2D TexCoord;
    int32 BoneIndices[4];
    float BoneWeights[4];

    void SkinningVertex(const TArray<FBone>& bones);

    void Serialize(FArchive& Ar) const
    {
        Ar << Position << Normal << Tangent << TexCoord;
        Ar << BoneIndices[0] << BoneIndices[1] << BoneIndices[2] << BoneIndices[3];
        Ar << BoneWeights[0] << BoneWeights[1] << BoneWeights[2] << BoneWeights[3];
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> Position >> Normal >> Tangent >> TexCoord;
        Ar >> BoneIndices[0] >> BoneIndices[1] >> BoneIndices[2] >> BoneIndices[3];
        Ar >> BoneWeights[0] >> BoneWeights[1] >> BoneWeights[2] >> BoneWeights[3];
    }
    
private:
    FVector SkinVertexPosition(const TArray<FBone>& bones) const;
};


struct FRefSkeletal
{
    // Tree structure for bones
    FString Name;
    TArray<FSkeletalVertex> RawVertices;
    TArray<FBone> RawBones;
    TArray<FBoneNode> BoneTree;
    TArray<int> RootBoneIndices;  // Indices of root bones (no parents)
    TMap<FString, int> BoneNameToIndexMap;  // For quick lookups
    TArray<UMaterial*> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    void Serialize(FArchive& Ar) const;

    void Deserialize(FArchive& Ar);
};

struct FSkeletalMeshRenderData
{
    // @todo PreviewName과 FilePath 분리하기
    FString Name = "Empty";
    TArray<FSkeletalVertex> Vertices;
    TArray<uint32> Indices;
    TArray<FBone> Bones;
    FBoundingBox BoundingBox;
    ID3D11Buffer* VB = nullptr;
    ID3D11Buffer* IB = nullptr;

    void Serialize(FArchive& Ar) const
    {
        Ar << Name;
        Ar << Vertices;
        Ar << Indices;
        Ar << Bones;
        Ar << BoundingBox;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> Name >> Vertices >> Indices >> Bones >> BoundingBox;
        // 2) 기존 버퍼가 있으면 해제
        if (VB) { VB->Release(); VB = nullptr; }
        if (IB) { IB->Release(); IB = nullptr; }

        // 3) 버텍스 버퍼 생성
        if (!Vertices.IsEmpty())
        {
            D3D11_BUFFER_DESC bd = {};
            bd.Usage          = D3D11_USAGE_DEFAULT;
            bd.ByteWidth      = UINT(sizeof(FSkeletalVertex) * Vertices.Num());
            bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = Vertices.GetData();

            HRESULT hr = GEngineLoop.GraphicDevice.Device->CreateBuffer(&bd, &initData, &VB);
            if (FAILED(hr))
            {
                // 에러 처리
                VB = nullptr;
                // UE_LOG(LogTemp, Error, TEXT("VB 생성 실패: 0x%08x"), hr);
            }
        }

        // 4) 인덱스 버퍼 생성
        if (!Indices.IsEmpty())
        {
            D3D11_BUFFER_DESC bd = {};
            bd.Usage          = D3D11_USAGE_DEFAULT;
            bd.ByteWidth      = UINT(sizeof(uint32) * Indices.Num());
            bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
            bd.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = Indices.GetData();

            HRESULT hr = GEngineLoop.GraphicDevice.Device->CreateBuffer(&bd, &initData, &IB);
            if (FAILED(hr))
            {
                IB = nullptr;
                // UE_LOG(LogTemp, Error, TEXT("IB 생성 실패: 0x%08x"), hr);
            }
        }
    }
};
#pragma endregion