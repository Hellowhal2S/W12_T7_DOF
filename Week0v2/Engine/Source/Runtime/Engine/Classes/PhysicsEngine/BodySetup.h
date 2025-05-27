#pragma once
#include "AggregateGeom.h"
#include "BodySetupCore.h"
#include "UObject/Object.h"

class UBodySetup : public UBodySetupCore
{
    DECLARE_CLASS(UBodySetup, UBodySetupCore)
public:
    UBodySetup() = default;
    ~UBodySetup() {} 

    FKAggregateGeom AggGeom;

    void Serialize(FArchive& Ar)
    {
        Ar << BoneName;
        Ar << AggGeom;
    }

    void Deserialize(FArchive& Ar)
    {
        Ar >> BoneName;
        Ar >> AggGeom;
    }
};



