#pragma once
#include "AggregateGeom.h"
#include "BodySetupCore.h"
#include "UObject/Object.h"

class UBodySetup : public UBodySetupCore
{
    DECLARE_CLASS(UBodySetup, UBodySetupCore)
public:
    UBodySetup() = default;
    ~UBodySetup() = default;

    FKAggregateGeom AggGeom;
};
