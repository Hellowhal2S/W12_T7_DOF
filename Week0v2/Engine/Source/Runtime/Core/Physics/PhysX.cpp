#include "PhysX.h"

#include "PhysXCallback.h"

PxDefaultAllocator      gAllocator;
PxDefaultErrorCallback  gErrorCallback;
PxFoundation*           gFoundation = nullptr;
PxPhysics*              gPhysics = nullptr;
PxScene*                gScene = nullptr;
PxMaterial*             gMaterial = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
MySimulationEventCallback* gMyCallback = nullptr;

std::vector<FGameObject> gObjects;