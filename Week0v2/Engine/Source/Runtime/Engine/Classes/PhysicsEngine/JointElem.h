#pragma once
#include <extensions/PxD6Joint.h>

#include "Math/Quat.h"
#include "Math/Vector.h"

using namespace physx;

enum class EJointMotion : uint8
{
    Locked,
    Limited,
    Free
};

struct FKJointElem
{
    FVector Center;
    FString ParentBoneName;
    FString ChildBoneName;
    FQuat Orientation;

    // Radian 값
    float TwistLimitMin;
    float TwistLimitMax;
    float SwingLimitMin;
    float SwingLimitMax;

     // eX      = 0,	//!< motion along the X axis
     // eY      = 1,	//!< motion along the Y axis
     // eZ      = 2,	//!< motion along the Z axis
     // eTWIST  = 3,	//!< motion around the X axis
     // eSWING1 = 4,	//!< motion around the Y axis
     // eSWING2 = 5,	//!< motion around the Z axis
    EJointMotion AxisMotions[6];
    
    void Serialize(FArchive& Ar) const
    {
        Ar << Center << ParentBoneName << ChildBoneName << Orientation
            << TwistLimitMin << TwistLimitMax << SwingLimitMin << SwingLimitMax;
        for (const EJointMotion& motion : AxisMotions)
        {
            uint8 value = static_cast<uint8>(motion);
            Ar << value;
        }
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> Center >> ParentBoneName >> ChildBoneName >> Orientation
           >> TwistLimitMin >> TwistLimitMax >> SwingLimitMin >> SwingLimitMax;
        for (EJointMotion& motion : AxisMotions)
        {
            uint8 value;
            Ar >> value;
            motion = static_cast<EJointMotion>(value);
        }
    }
};
