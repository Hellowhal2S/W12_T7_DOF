#include "BodyInstance.h"

#include "Physics/PhysX.h"

FBodyInstance::FBodyInstance(UPrimitiveComponent* InOwnerComponent, EBodyType InBodyType, const PxVec3& InPos)
{
    OwnerComponent = InOwnerComponent;
    PxTransform Pose(InPos);
    if (InBodyType == EBodyType::Dynamic)
    {
        RigidDynamicHandle = gPhysics->createRigidDynamic(Pose);
        RigidActorHandle = RigidDynamicHandle;
    }
    else
    {
        RigidStaticHandle = gPhysics->createRigidStatic(Pose);
        RigidActorHandle = RigidStaticHandle;
    }
}
