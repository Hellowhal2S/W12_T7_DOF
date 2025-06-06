#include "PhysX.h"

#include "PhysXCallback.h"

PxDefaultAllocator      gAllocator;
PxDefaultErrorCallback  gErrorCallback;
PxFoundation*           gFoundation = nullptr;
PxPhysics*              gPhysics = nullptr;
PxMaterial*             gMaterial = nullptr;
PxPvd*                  gPvd = nullptr;
PxPvdTransport*         gTransport = nullptr;

PxDefaultCpuDispatcher* gDispatcher = nullptr;
MySimulationEventCallback* gMyCallback = nullptr;
PxCooking*                 gCooking = nullptr;

