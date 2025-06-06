﻿#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UConstraintSetup;
class UBodySetup;

class UPhysicsAsset : public UObject
{
    DECLARE_CLASS(UPhysicsAsset, UObject)
public:
    UPhysicsAsset() = default;
    ~UPhysicsAsset() = default;

    TArray<UBodySetup*> BodySetup;
    TArray<UConstraintSetup*> ConstraintSetup;

    TMap<FName, int32> BodySetupIndexMap;

    void UpdateBodySetupIndexMap();
    
    void Serialize(FArchive& Ar);
    void Deserialize(FArchive& Ar);
};
