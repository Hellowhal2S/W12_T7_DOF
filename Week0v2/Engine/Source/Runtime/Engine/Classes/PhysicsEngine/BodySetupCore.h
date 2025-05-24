#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UBodySetupCore : public UObject
{
    DECLARE_CLASS(UBodySetupCore, UObject)
public:
    UBodySetupCore() = default;
    ~UBodySetupCore() = default;

    FName BoneName;
};
