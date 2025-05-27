#pragma once
#include <PxRigidDynamic.h>
#include <PxShape.h>

#include "BodyInstanceCore.h"
#include "Container/Array.h"
#include "UObject/NameTypes.h"

enum class EBodyType { Dynamic, Static };
using namespace physx;
class UPrimitiveComponent;

struct FBodyInstance : public FBodyInstanceCore
{
    PxRigidActor* RigidActorHandle = nullptr;
    PxRigidDynamic* RigidDynamicHandle = nullptr;
    PxRigidStatic* RigidStaticHandle = nullptr;
    
    TArray<PxShape*> ShapeHandle;
    
    UPrimitiveComponent* OwnerComponent;
    FName BoneName;

    FBodyInstance() : OwnerComponent(nullptr) {}
    FBodyInstance(UPrimitiveComponent* InOwnerComponent, EBodyType InBodyType, const PxVec3& InPos, FName InBoneName);
    ~FBodyInstance()=default;

    //void* PxRigidActor->userData
    //void* PxShape->userData
public:
    
    const char* GetConcreteTypeName() const
    {
        if (RigidDynamicHandle)
        {
            return "PxRigidDynamic";
        }
        else
        {
            return "PxRigidStatic";
        }
    }
    
    //
    //  RigidDynamic
    //
    void SetKinematicTarget(const PxTransform& destination)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setKinematicTarget(destination);
        }
    }

    bool GetKinematicTarget(PxTransform& target)
    {
        if (RigidDynamicHandle)
        {
            return RigidDynamicHandle->getKinematicTarget(target);
        }
        return false;
    }

    bool IsSleeping() const
    {
        return RigidDynamicHandle && RigidDynamicHandle->isSleeping();
    }

    void SetSleepThreshold(PxReal threshold)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setSleepThreshold(threshold);
        }
    }

    PxReal GetSleepThreshold() const
    {
        if (RigidDynamicHandle)
        {
            return RigidDynamicHandle->getSleepThreshold();
        }
        return 0.f;
    }

    void SetStabilizationThreshold(PxReal threshold)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setStabilizationThreshold(threshold);
        }
    }

    PxReal GetStabilizationThreshold() const
    {
        if (RigidDynamicHandle)
        {
            return RigidDynamicHandle->getStabilizationThreshold();
        }
        return 0.f;
    }

    PxRigidDynamicLockFlags GetRigidDynamicLockFlags() const
    {
        if (RigidDynamicHandle)
        {
            return RigidDynamicHandle->getRigidDynamicLockFlags();
        }
        return PxRigidDynamicLockFlags(0);
    }

    void SetRigidDynamicLockFlag(PxRigidDynamicLockFlag::Enum flag, bool value)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setRigidDynamicLockFlag(flag, value);
        }
    }

    void SetRigidDynamicLockFlags(PxRigidDynamicLockFlags flags)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setRigidDynamicLockFlags(flags);
        }
    }

    void SetWakeCounter(PxReal wakeCounterValue)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setWakeCounter(wakeCounterValue);
        }
    }

    PxReal GetWakeCounter() const
    {
        if (RigidDynamicHandle)
        {
            return RigidDynamicHandle->getWakeCounter();
        }
        return 0.f;
    }

    void WakeUp()
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->wakeUp();
        }
    }

    void PutToSleep()
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->putToSleep();
        }
    }

    void SetSolverIterationCounts(PxU32 minPositionIters, PxU32 maxVelocityIters = 1)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setSolverIterationCounts(minPositionIters, maxVelocityIters);
        }
    }

    void GetSolverIterationCounts(PxU32& minPositionIters, PxU32& minVelocityIters) const
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->getSolverIterationCounts(minPositionIters, minVelocityIters);
        }
    }

    PxReal GetContactReportThreshold() const
    {
        if (RigidDynamicHandle)
        {
            return RigidDynamicHandle->getContactReportThreshold();
        }
        return 0.f;
    }

    void SetContactReportThreshold(PxReal threshold)
    {
        if (RigidDynamicHandle)
        {
            RigidDynamicHandle->setContactReportThreshold(threshold);
        }
    }
    
    //
    // RigidActor
    //
    void Release()
    {
        if (RigidActorHandle)
        {
            RigidActorHandle->userData = nullptr;
            RigidActorHandle->release();
        }
    }

    PxTransform GetGlobalPose() const
    {
        return RigidActorHandle->getGlobalPose();
    }

    void SetGlobalPose(const PxTransform& pose, bool autowake = true)
    {
        RigidActorHandle->setGlobalPose(pose, autowake);
    }

    bool AttachShape(PxShape& shape)
    {
        return RigidActorHandle->attachShape(shape);
    }

    void DetachShape(PxShape& shape, bool wakeOnLostTouch = true)
    {
        RigidActorHandle->detachShape(shape, wakeOnLostTouch);
    }

    PxU32 GetNbShapes() const
    {
        return RigidActorHandle->getNbShapes();
    }

    PxU32 GetShapes(PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const
    {
        return RigidActorHandle->getShapes(userBuffer, bufferSize, startIndex);
    }

    PxU32 GetNbConstraints() const
    {
        return RigidActorHandle->getNbConstraints();
    }

    PxU32 GetConstraints(PxConstraint** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const
    {
        return RigidActorHandle->getConstraints(userBuffer, bufferSize, startIndex);
    }

    //
    // PxActor
    //
    PxActorType::Enum GetType() const
    {
        return RigidActorHandle->getType();
    }

    PxScene* GetScene() const
    {
        return RigidActorHandle->getScene();
    }

    void SetName(const char* name)
    {
        RigidActorHandle->setName(name);
    }

    const char* GetName() const
    {
        return RigidActorHandle->getName();
    }

    PxBounds3 GetWorldBounds(float inflation=1.01f) const
    {
        return RigidActorHandle->getWorldBounds(inflation);
    }

    void SetActorFlag(PxActorFlag::Enum flag, bool value)
    {
        RigidActorHandle->setActorFlag(flag, value);
    }

    void SetActorFlags(PxActorFlags flags)
    {
        RigidActorHandle->setActorFlags(flags);
    }

    PxActorFlags GetActorFlags() const
    {
        return RigidActorHandle->getActorFlags();
    }

    void SetDominanceGroup(PxDominanceGroup dominanceGroup)
    {
        RigidActorHandle->setDominanceGroup(dominanceGroup);
    }

    PxDominanceGroup GetDominanceGroup() const
    {
        return RigidActorHandle->getDominanceGroup();
    }

    void SetOwnerClient(PxClientID& inClient)
    {
        RigidActorHandle->setOwnerClient(inClient);
    }

    PxClientID GetOwnerClient() const
    {
        return RigidActorHandle->getOwnerClient();
    }

    PxAggregate* GetAggregate() const
    {
        return RigidActorHandle->getAggregate();
    }
};