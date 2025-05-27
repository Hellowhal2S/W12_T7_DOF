#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UConstraintSetupCore : public UObject
{
    DECLARE_CLASS(UConstraintSetupCore, UObject)
public:
    UConstraintSetupCore() = default;
    ~UConstraintSetupCore() = default;
    
    FName JointName;
};
