#pragma once

#include "Math/Transform.h"

#include <d3d11.h>
#include <vector>
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>


struct FVector;
using namespace physx;
using namespace DirectX;

extern PxDefaultAllocator      gAllocator;
extern PxDefaultErrorCallback  gErrorCallback;
extern PxFoundation*           gFoundation;
extern PxPhysics*              gPhysics;
extern PxScene*                gScene;
extern PxMaterial*             gMaterial;
extern PxDefaultCpuDispatcher* gDispatcher;

struct FGameObject {
    PxRigidDynamic* rigidBody = nullptr;
    XMMATRIX worldMatrix = XMMatrixIdentity();

    void UpdateFromPhysics() {
        PxTransform t = rigidBody->getGlobalPose();
        PxMat44 mat(t);
        worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&mat));
    }
};
// TODO : 충분한 논의가 필요함
 // PxVec3 = FVector
 // PxQuat = FQuat
 // PxTransform = FTransform
 // PxRaycastHit = FHitResult;
 // PxOverlapHit = FOverlapResult
 // PxSweepHit = FSweepResult
 // PxFilterData = FMaskFilter
 // PxMaterial = UPhysicalMaterial
 // PxShape = FBodyInstance
 // PxRigidActor = FBodyInstance
 // PxRigidDynamic = FBodyInstance
 // PxRigidStatic = FBodyInstance
 // PxJoint = FConstraintInstance
 // PxScene = UWorld->GetPhysicsScene()

extern std::vector<FGameObject> gObjects;

struct FPhysX
{
    static void InitPhysX() {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale());
        gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0, -9.81f, 0);
        gDispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        gScene = gPhysics->createScene(sceneDesc);
    }

    static FGameObject CreateBox(const PxVec3& pos, const PxVec3& halfExtents) {
        FGameObject obj;
        PxTransform pose(pos);
        obj.rigidBody = gPhysics->createRigidDynamic(pose);
        PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
        obj.rigidBody->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*obj.rigidBody, 10.0f);
        gScene->addActor(*obj.rigidBody);
        obj.UpdateFromPhysics();
        return obj;
    }

    static FGameObject CreateSphere(const PxVec3& pos, float radius)
    {
        FGameObject obj;
        PxTransform pose(pos);
        obj.rigidBody = gPhysics->createRigidDynamic(pose);
        PxShape* shape = gPhysics->createShape(PxSphereGeometry(radius), *gMaterial);
        obj.rigidBody->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*obj.rigidBody, 10.0f);
        gScene->addActor(*obj.rigidBody);
        obj.UpdateFromPhysics();
        return obj;
    }

    static FGameObject CreateCapsule(const PxVec3& pos, float radius, float halfHeight)
    {
        FGameObject obj;
        PxTransform pose(pos);
        obj.rigidBody = gPhysics->createRigidDynamic(pose);
        PxShape* shape = gPhysics->createShape(PxCapsuleGeometry(radius, halfHeight), *gMaterial);
        obj.rigidBody->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*obj.rigidBody, 10.0f);
        gScene->addActor(*obj.rigidBody);
        return obj;
    }
    
    static void Simulate(float dt) {
        gScene->simulate(dt);
        gScene->fetchResults(true);
        for (auto& obj : gObjects) obj.UpdateFromPhysics();
    }

    static PxDefaultAllocator& GetPhysXAllocator() {
        return gAllocator;
    }

    static PxDefaultErrorCallback& GetPhysXErrorCallback() {
        return gErrorCallback;
    }

    static PxFoundation* GetPhysXFoundation() {
        return gFoundation;
    }

    static PxPhysics* GetPhysXSDK() {
        return gPhysics;
    }

    static PxScene* GetPhysXScene() {
        return gScene;
    }

    static PxMaterial* GetDefaultPhysXMaterial() {
        return gMaterial;
    }

    static PxCpuDispatcher* GetPhysXDispatcher() {
        return gDispatcher;
    }
};
