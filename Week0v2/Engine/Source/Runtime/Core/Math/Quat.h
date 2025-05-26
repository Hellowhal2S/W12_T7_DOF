#pragma once
#include "Matrix.h"
#include "Vector.h"
#include "Serialization/Archive.h"

// 쿼터니언
struct FQuat
{
    float W, X, Y, Z;

    // 기본 생성자
    explicit FQuat()
        : W(1.0f), X(0.0f), Y(0.0f), Z(0.0f)
    {}

    // FQuat 생성자 추가: 회전 축과 각도를 받아서 FQuat 생성
    explicit FQuat(const FVector& Axis, float Angle);

    // W, X, Y, Z 값으로 초기화
    explicit FQuat(float InW, float InX, float InY, float InZ)
        : W(InW), X(InX), Y(InY), Z(InZ)
    {}

    explicit FQuat(const FMatrix& InMatrix);

    const static FQuat Identity;

    void Serialize(FArchive& Ar) const
    {
        Ar << W << X << Y << Z;
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> W >> X >> Y >> Z;
    }

    // 쿼터니언의 곱셈 연산 (회전 결합)
    FQuat operator*(const FQuat& Other) const;

    // (쿼터니언) 벡터 회전
    FVector RotateVector(const FVector& Vec) const;

    // 단위 쿼터니언 여부 확인
    bool IsNormalized() const;

    // 쿼터니언 정규화 (단위 쿼터니언으로 만듬)
    void Normalize(float Tolerance = SMALL_NUMBER);

    FQuat GetUnsafeNormal() const;
    FQuat GetSafeNormal(float Tolerance = SMALL_NUMBER) const;

    // 회전 각도와 축으로부터 쿼터니언 생성 (axis-angle 방식)
    static FQuat FromAxisAngle(const FVector& Axis, float Angle);

    static FQuat CreateRotation(float roll, float pitch, float yaw);

    // 쿼터니언을 회전 행렬로 변환
    FMatrix ToMatrix() const;

    bool Equals(const FQuat& Q, float Tolerance = KINDA_SMALL_NUMBER) const;

    FRotator Rotator() const;

    float AngularDistance(const FQuat& Q) const;
    
    static FQuat Slerp(const FQuat& Quat1, const FQuat& Quat2, float Slerp);
    
    static FQuat Slerp_NotNormalized(const FQuat& Quat1, const FQuat& Quat2, float Slerp);

    static FQuat FindBetweenNormals(const FVector& A, const FVector& B)
    {
        FVector v0 = A.GetSafeNormal();
        FVector v1 = B.GetSafeNormal();

        float dot = v0.Dot(v1);

        // 벡터가 완전히 반대인 경우 (180도 회전)
        if (dot < -1.0f + KINDA_SMALL_NUMBER)
        {
            FVector orthogonal = FVector(1.0f, 0.0f, 0.0f).Cross(v0);
            if (orthogonal.MagnitudeSquared() < KINDA_SMALL_NUMBER)
            {
                orthogonal = FVector(0.0f, 1.0f, 0.0f).Cross(v0);
            }
            orthogonal.Normalize();
            return FQuat::FromAxisAngle(orthogonal, PI);
        }

        FVector cross = v0.Cross(v1);
        float s = sqrtf((1 + dot) * 2);
        float invs = 1.0f / s;

        FQuat q;
        q.W = s * 0.5f;
        q.X = cross.X * invs;
        q.Y = cross.Y * invs;
        q.Z = cross.Z * invs;
        return q.GetSafeNormal();

    }

};

inline const FQuat FQuat::Identity = FQuat(1.0f, 0.0f, 0.0f, 0.0f);
