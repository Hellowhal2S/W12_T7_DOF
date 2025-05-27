#pragma once
#include "ConstraintSetupCore.h"
#include "JointElem.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UConstraintSetup : public UConstraintSetupCore
{
    DECLARE_CLASS(UConstraintSetup, UConstraintSetupCore)
public:
    UConstraintSetup() = default;
    ~UConstraintSetup() = default;

    TArray<FKJointElem> JointElems;

    void Serialize(FArchive& Ar)
    {
        Ar << JointName;
        Ar << JointElems;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> JointName;
        Ar >> JointElems;
    }
};
