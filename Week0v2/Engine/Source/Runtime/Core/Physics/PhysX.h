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
extern PxPvd* gPvd;
extern PxPvdTransport* gTransport;

struct FPhysX
{
    static void InitPhysX() {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
        
        gPvd = PxCreatePvd(*gFoundation);
        gTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        if (!gPvd || !gTransport)
        {
            printf("fail : (gPvd: %p, gTransport: %p)\n", gPvd, gTransport);
            return;
        }
        bool success = gPvd->connect(*gTransport, PxPvdInstrumentationFlag::eALL);
        if (!success)
        {
            printf("fail : PVD connect() \n");
        }
        

        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
        gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
        


        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0, 0, -9.81f);
        gDispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        gScene = gPhysics->createScene(sceneDesc);
        
        PxPvdSceneClient* PvdClient = gScene->getScenePvdClient();
        if (PvdClient)
        {
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }

        PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
        
        PxRigidStatic* GroundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 0, 1, 0), *gMaterial);
        gScene->addActor(*GroundPlane);
        
        PxTransform BoxTransform(PxVec3(0.0f, 0.0f, 100.0f)); 
        PxBoxGeometry BoxGeometry(PxVec3(1.0f, 1.0f, 1.0f)); 

        PxRigidDynamic* BoxActor = PxCreateDynamic(*gPhysics, BoxTransform, BoxGeometry, *gMaterial, 10.0f);
        BoxActor->setAngularDamping(0.5f);
        BoxActor->setLinearDamping(0.5f);
        BoxActor->setMass(10.0f);

        gScene->addActor(*BoxActor);
        printf("Init Physics Scene\n");
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
