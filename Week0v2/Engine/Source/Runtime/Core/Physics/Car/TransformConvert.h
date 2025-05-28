#pragma once
#include <PxPhysicsAPI.h>
#include "Math/Transform.h"

inline physx::PxVec3 U2PVector(const FVector& v)
{
    return physx::PxVec3(v.X * 0.01f, v.Y * 0.01f, v.Z * 0.01f);
}

inline physx::PxQuat U2PQuat(const FQuat& q)
{
    return physx::PxQuat(q.X, q.Y, q.Z, q.W);
}

inline physx::PxTransform U2PTransform(const FTransform& t)
{
    return physx::PxTransform(
        U2PVector(t.GetLocation()),
        U2PQuat(t.GetRotation())
    );
}

inline FVector P2UVector(const physx::PxVec3& v)
{
    return FVector(v.x * 100.0f,   // m→cm
                   v.y * 100.0f,
                   v.z * 100.0f);
}

// PhysX 쿼터니언 (x,y,z,w) 그대로 매핑
inline FQuat P2UQuat(const physx::PxQuat& q)
{
    return FQuat(q.x, q.y, q.z, q.w);
}