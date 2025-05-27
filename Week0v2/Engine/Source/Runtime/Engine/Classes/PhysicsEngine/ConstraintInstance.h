#pragma once
#include <extensions/PxJoint.h>

#include "UObject/NameTypes.h"

class UPrimitiveComponent;
using namespace physx;

struct FConstraintInstance
{
    PxJoint* JointHandle = nullptr;

    UPrimitiveComponent* OwnerComponent;
    FName ConstraintName;

    FConstraintInstance(UPrimitiveComponent* InOwnerComponent, FName InConstraintName, PxD6Joint* InJoint);
    ~FConstraintInstance()=default;

    //void* PxJoint->userData
public:
    void SetActors(PxRigidActor* actor0, PxRigidActor* actor1)
    {
        JointHandle->setActors(actor0, actor1);
    }

    void GetActors(PxRigidActor*& actor0, PxRigidActor*& actor1) const
    {
        JointHandle->getActors(actor0, actor1);
    }

    void SetLocalPose(PxJointActorIndex::Enum actor, const PxTransform& localpose)
    {
        JointHandle->setLocalPose(actor, localpose);
    }

    PxTransform GetLocalPose(PxJointActorIndex::Enum actor) const
    {
        return JointHandle->getLocalPose(actor);
    }

    PxTransform GetRelativeTransform() const
    {
        return JointHandle->getRelativeTransform();
    }

    PxVec3 GetRelativeLinearVelocity() const
    {
        return JointHandle->getRelativeLinearVelocity();
    }

    PxVec3 GetRelativeAngularVelocity() const
    {
        return JointHandle->getRelativeAngularVelocity();
    }

    void SetBreakForce(PxReal force, PxReal torque)
    {
        JointHandle->setBreakForce(force, torque);
    }

    void GetBreakForce(PxReal& force, PxReal& torque) const
    {
        JointHandle->getBreakForce(force, torque);
    }

    void SetConstraintFlags(PxConstraintFlags flags)
    {
        JointHandle->setConstraintFlags(flags);
    }

    void SetConstraintFlag(PxConstraintFlag::Enum flag, bool value)
    {
        JointHandle->setConstraintFlag(flag, value);
    }

    PxConstraintFlags GetConstraintFlags() const
    {
        return JointHandle->getConstraintFlags();
    }

    void SetInvMassScale0(PxReal invMassScale)
    {
        JointHandle->setInvMassScale0(invMassScale);
    }

    PxReal GetInvMassScale0() const
    {
        return JointHandle->getInvMassScale0();
    }

    void SetInvInertiaScale0(PxReal invInertialScale)
    {
        JointHandle->setInvInertiaScale0(invInertialScale);
    }

    PxReal GetInvInertiaScale0() const
    {
        return JointHandle->getInvInertiaScale0();
    }

    void SetInvMassScale1(PxReal invMassScale)
    {
        JointHandle->setInvMassScale1(invMassScale);
    }

    PxReal GetInvMassScale1() const
    {
        return JointHandle->getInvMassScale1();
    }

    void setInvInertiaScale1(PxReal invInertiaScale)
    {
        JointHandle->setInvInertiaScale1(invInertiaScale);
    }

    PxReal GetInvInertiaScale1() const
    {
        return JointHandle->getInvInertiaScale1();
    }

    PxConstraint* GetConstraint() const
    {
        return JointHandle->getConstraint();
    }

    void SetName(const char* name)
    {
        JointHandle->setName(name);
    }

    const char* GetName() const
    {
        return JointHandle->getName();
    }

    void Release()
    {
        JointHandle->release();
    }

    PxScene* GetScene() const
    {
        return JointHandle->getScene();
    }

    void* userData;

    static void GetBinaryMetaData(PxOutputStream& stream)
    {
        PxJoint::getBinaryMetaData(stream);
    }
};