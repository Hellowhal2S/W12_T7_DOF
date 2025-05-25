#include "PhysX.h"

PxDefaultAllocator      gAllocator;
PxDefaultErrorCallback  gErrorCallback;
PxFoundation*           gFoundation = nullptr;
PxPhysics*              gPhysics = nullptr;
PxScene*                gScene = nullptr;
PxMaterial*             gMaterial = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxPvd* gPvd = nullptr;
PxPvdTransport* gTransport = nullptr;

std::vector<FGameObject> gObjects;


