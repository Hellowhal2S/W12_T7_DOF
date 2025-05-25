#pragma once

#include "Math/Transform.h"

#include <d3d11.h>
#include <vector>
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>
#include "PhysXCallback.h"

#define SCOPED_READ_LOCK(scene) physx::PxSceneReadLock readLock(scene);

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
extern MySimulationEventCallback* gMyCallback;

struct FGameObject {
    PxRigidDynamic* rigidBody = nullptr;
    XMMATRIX worldMatrix = XMMatrixIdentity();

    void UpdateFromPhysics() {
        SCOPED_READ_LOCK(*gScene);
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

namespace CollisionGroup {
    enum Enum : physx::PxU32 {
        DEFAULT     = (1<<0),
        ENVIRONMENT = (1<<1),
        CHARACTER   = (1<<2),
    };
}

inline physx::PxFilterData MakeFilterData(
    CollisionGroup::Enum group,
    physx::PxU32        collideMask
){
    physx::PxFilterData fd;
    fd.word0 = static_cast<physx::PxU32>(group);  // 이 Shape의 그룹
    fd.word1 = collideMask;                       // 충돌 허용 마스크
    fd.word2 = 0;                                 // 필요시 Query 용도로 사용
    fd.word3 = 0;
    return fd;
}

struct FPhysX
{
    static void InitPhysX() {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale());
        gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0, -9.81f, 0);
        gDispatcher = PxDefaultCpuDispatcherCreate(4);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;;
        sceneDesc.filterShader = MyFilterShader;
        gMyCallback = new MySimulationEventCallback();
        sceneDesc.simulationEventCallback = gMyCallback;
        gScene = gPhysics->createScene(sceneDesc);
    }

    static PxFilterFlags MyFilterShader(
        PxFilterObjectAttributes attr0, PxFilterData fd0,
        PxFilterObjectAttributes attr1, PxFilterData fd1,
        PxPairFlags& pairFlags,
        const void* constantBlock,
        PxU32 constantBlockSize)
    {
        // 1) 트리거 판정
        if (PxFilterObjectIsTrigger(attr0) || PxFilterObjectIsTrigger(attr1)) {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return PxFilterFlag::eDEFAULT;
        }
        // 2) 그룹 검사
        if (!((fd0.word0 & fd1.word1) &&
              (fd1.word0 & fd0.word1)))
            return PxFilterFlag::eSUPPRESS;
        // 3) Contact + Notification 플래그
        pairFlags  = PxPairFlag::eCONTACT_DEFAULT
                   | PxPairFlag::eNOTIFY_TOUCH_FOUND
                   | PxPairFlag::eNOTIFY_TOUCH_PERSISTS
                   | PxPairFlag::eNOTIFY_TOUCH_LOST;
        return PxFilterFlag::eDEFAULT;
    }

    static FGameObject CreateBox(const PxVec3& pos, const PxVec3& halfExtents) {
        FGameObject obj;
        PxTransform pose(pos);
        obj.rigidBody = gPhysics->createRigidDynamic(pose);
        PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
        PxFilterData fd = MakeFilterData(CollisionGroup::ENVIRONMENT, CollisionGroup::DEFAULT | CollisionGroup::ENVIRONMENT | CollisionGroup::CHARACTER);
        shape->setSimulationFilterData(fd);
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
        PxFilterData fd = MakeFilterData(CollisionGroup::ENVIRONMENT, CollisionGroup::DEFAULT | CollisionGroup::ENVIRONMENT | CollisionGroup::CHARACTER);
        shape->setSimulationFilterData(fd);
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
        PxFilterData fd = MakeFilterData(CollisionGroup::ENVIRONMENT, CollisionGroup::DEFAULT | CollisionGroup::ENVIRONMENT | CollisionGroup::CHARACTER);
        shape->setSimulationFilterData(fd);
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
