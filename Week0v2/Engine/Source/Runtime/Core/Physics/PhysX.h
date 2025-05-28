#pragma once

#include <PxPhysicsAPI.h>
#include "PhysXCallback.h"
#include "UserInterface/Console.h"
#include "vehicle/PxVehicleSDK.h"

enum class LogLevel;
struct FVector;
using namespace physx;

extern PxDefaultAllocator      gAllocator;
extern PxDefaultErrorCallback  gErrorCallback;
extern PxFoundation*           gFoundation;
extern PxPhysics*              gPhysics;
extern PxCooking*              gCooking;
extern PxPvd*                  gPvd;
extern PxPvdTransport*         gTransport;
extern PxMaterial*             gMaterial;
extern PxDefaultCpuDispatcher* gDispatcher;
extern MySimulationEventCallback* gMyCallback;
extern PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs;

struct FPhysX
{
    enum class EActorType : PxU32 {
        Dynamic = 0,
        Static  = 1
    };
    
    enum class ECollisionGroup : PxU32 {
        Default     = (1<<0),
        Player      = (1<<1),
        Enemy       = (1<<2),
        Environment = (1<<3),
        BoxCollider = (1<<4),
        VehicleBody = (1<<5),
        VehicleWheel = (1<<6),
        All         = Default | Player | Enemy | Environment | BoxCollider | VehicleBody | VehicleWheel
    };

    static PxFilterData MakeFilterData(
        ECollisionGroup group,
        ECollisionGroup mask)
    {
        PxFilterData fd;
        fd.word0 = static_cast<PxU32>(group);
        fd.word1 = static_cast<PxU32>(mask);
        return fd;
    }
    
    static PxFilterFlags MyFilterShader(
        PxFilterObjectAttributes attr0, PxFilterData fd0,
        PxFilterObjectAttributes attr1, PxFilterData fd1,
        PxPairFlags& pairFlags,
        const void* constantBlock,
        PxU32 constantBlockSize)
    {
        // 1) 트리거 판정
        if (PxFilterObjectIsTrigger(attr0) || PxFilterObjectIsTrigger(attr1))
        {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return PxFilterFlag::eDEFAULT;
        }
        // 2) 그룹 검사
        if (!((fd0.word0 & fd1.word1) && (fd1.word0 & fd0.word1)))
            return PxFilterFlag::eSUPPRESS;
        // 3) Contact + Notification 플래그
        pairFlags  = PxPairFlag::eCONTACT_DEFAULT
                   | PxPairFlag::eNOTIFY_TOUCH_FOUND
                   | PxPairFlag::eNOTIFY_TOUCH_PERSISTS
                   | PxPairFlag::eNOTIFY_TOUCH_LOST;
        
        // return: 이 페어를 accept(eDEFAULT)할지, ignore(eSUPPRESS)할지 결정
        return PxFilterFlag::eDEFAULT;
    }
    
    static void InitPhysX()
    {
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

        PxTolerancesScale scale = gPhysics->getTolerancesScale();
        PxCookingParams cookParams(scale);
        // (선택) 메시 전처리 플래그 설정
        cookParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eWELD_VERTICES;
        cookParams.buildGPUData       = true;
        gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, cookParams);
        if (!gCooking)
        {
            UE_LOG(LogLevel::Display, TEXT("PxCreateCooking failed!"));
            return;
        }

        gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

        if (!InitVehicleSDK())
        {
            printf("Error: Vehicle SDK initialization failed\n");
        }
    }

    static void ShutdownVehicleSDK()
    {
        if (gFrictionPairs)
        {
            gFrictionPairs->release();
            gFrictionPairs = nullptr;
        }
        PxCloseVehicleSDK();
    }

private:
    static bool InitVehicleSDK()
    {
        if (!PxInitVehicleSDK(*gPhysics))
        {
            printf("Error: PxInitVehicleSDK failed!\n");
            return false;
        }

        // Z-up, X-Forward 기준으로 설정
        PxVehicleSetBasisVectors(PxVec3(0,0,1), PxVec3(1,0,0)); 
        PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);

        // 기본 지면 타입 및 마찰력 설정
        PxVehicleDrivableSurfaceType surfaceTypes[1];
        surfaceTypes[0].mType = 0; // 기본 지면 타입
        const PxReal defaultFriction = 1.0f;
        gFrictionPairs = PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(1, 1); // 1개의 타이어 타입, 1개의 지면 타입
        const PxMaterial* drivableSurfaceMaterials[1] = {gMaterial}; // 지면 재질
        gFrictionPairs->setup(1, 1, drivableSurfaceMaterials, surfaceTypes);
        gFrictionPairs->setTypePairFriction(0, 0, defaultFriction); // 타이어 타입 0, 지면 타입 0 간의 마찰력
        
        return true;
    }
};

inline FPhysX::ECollisionGroup operator|(
    FPhysX::ECollisionGroup a,
    FPhysX::ECollisionGroup b
) {
    return static_cast<FPhysX::ECollisionGroup>(
        static_cast<PxU32>(a) | static_cast<PxU32>(b)
    );
}