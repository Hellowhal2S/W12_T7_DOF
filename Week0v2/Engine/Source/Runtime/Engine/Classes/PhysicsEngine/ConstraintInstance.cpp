#include "ConstraintInstance.h"

FConstraintInstance::FConstraintInstance(UPrimitiveComponent* InOwnerComponent, FName InConstraintName, PxD6Joint* InJoint)
{
    OwnerComponent = InOwnerComponent;
    ConstraintName = InConstraintName;
    JointHandle = reinterpret_cast<PxJoint*>(InJoint);
}
