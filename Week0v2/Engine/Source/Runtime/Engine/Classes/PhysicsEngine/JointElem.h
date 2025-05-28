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
    FName ParentBoneName; // 1
    FName ChildBoneName;  // 2

    FVector Center;
    FQuat Rotation;
    
    // Radian 값
    float TwistLimitMin;
    float TwistLimitMax;
    float SwingLimitMin1;
    float SwingLimitMax1;
    float SwingLimitMin2;
    float SwingLimitMax2;

     // eX      = 0,	//!< motion along the X axis
     // eY      = 1,	//!< motion along the Y axis
     // eZ      = 2,	//!< motion along the Z axis
     // eTWIST  = 3,	//!< motion around the X axis
     // eSWING1 = 4,	//!< motion around the Y axis
     // eSWING2 = 5,	//!< motion around the Z axis
    EJointMotion AxisMotions[6];
    
    void Serialize(FArchive& Ar) const
    {
        Ar << ParentBoneName << ChildBoneName << Center << Rotation
            << TwistLimitMin << TwistLimitMax << SwingLimitMin1 << SwingLimitMax1 << SwingLimitMin2 << SwingLimitMax2;
        for (const EJointMotion& motion : AxisMotions)
        {
            uint8 value = static_cast<uint8>(motion);
            Ar << value;
        }
    }
    void Deserialize(FArchive& Ar)
    {
        Ar >> ParentBoneName >> ChildBoneName >> Center >> Rotation
           >> TwistLimitMin >> TwistLimitMax >> SwingLimitMin1 >> SwingLimitMax1 >> SwingLimitMin2 >> SwingLimitMax2;
        for (EJointMotion& motion : AxisMotions)
        {
            uint8 value;
            Ar >> value;
            motion = static_cast<EJointMotion>(value);
        }
    }
};
