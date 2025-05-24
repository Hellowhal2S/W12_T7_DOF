#pragma once

#include <PxPhysicsAPI.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

#include "Math/Transform.h"

struct FVector;
using namespace physx;
using namespace DirectX;

PxDefaultAllocator      gAllocator;
PxDefaultErrorCallback  gErrorCallback;
PxFoundation*           gFoundation = nullptr;
PxPhysics*              gPhysics = nullptr;
PxScene*                gScene = nullptr;
PxMaterial*             gMaterial = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;

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

std::vector<FGameObject> gObjects;

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
