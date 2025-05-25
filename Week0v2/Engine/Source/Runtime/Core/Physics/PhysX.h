#pragma once

#include <PxPhysicsAPI.h>
#include "PhysXCallback.h"

struct FVector;
using namespace physx;

extern PxDefaultAllocator      gAllocator;
extern PxDefaultErrorCallback  gErrorCallback;
extern PxFoundation*           gFoundation;
extern PxPhysics*              gPhysics;
extern PxPvd*                  gPvd;
extern PxPvdTransport*         gTransport;
extern PxMaterial*             gMaterial;
extern PxDefaultCpuDispatcher* gDispatcher;
extern MySimulationEventCallback* gMyCallback;

struct FPhysX
{
    enum class ECollisionGroup : PxU32 {
        Default     = (1<<0),
        Player      = (1<<1),
        Enemy       = (1<<2),
        Environment = (1<<3),
        All         = Default | Player | Enemy | Environment
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
        
        // return: 이 페어를 accept(eDEFAULT)할지, ignore(eSUPPRESS)할지 결정
        return PxFilterFlag::eDEFAULT;
    }
    
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
    }
};
