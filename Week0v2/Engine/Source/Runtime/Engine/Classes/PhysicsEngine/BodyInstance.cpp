#include "BodyInstance.h"

#include "Physics/PhysX.h"

FBodyInstance::FBodyInstance(UPrimitiveComponent* InOwnerComponent, EBodyType InBodyType, const PxVec3& InPos, FName InBoneName)
{
    OwnerComponent = InOwnerComponent;
    BoneName = InBoneName;
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
