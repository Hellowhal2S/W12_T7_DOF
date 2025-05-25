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
