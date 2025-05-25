#pragma once
#include <PxPhysicsAPI.h>

class MySimulationEventCallback : public physx::PxSimulationEventCallback
{
public:
    void onContact(const physx::PxContactPairHeader&, const physx::PxContactPair*, physx::PxU32) override;
    virtual void onTrigger(physx::PxTriggerPair*, physx::PxU32) override;
    virtual void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override;
    virtual void onWake(physx::PxActor**, physx::PxU32) override;
    virtual void onSleep(physx::PxActor**, physx::PxU32) override;
    virtual void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, const physx::PxU32) override;
};
