#include "PhysicsAsset.h"
#include "BodySetup.h"
#include "ConstraintSetup.h"

void UPhysicsAsset::UpdateBodySetupIndexMap()
{
    // update BodySetupIndexMap
    BodySetupIndexMap.Empty();

    for(int32 i=0; i < BodySetup.Num(); i++)
    {
        BodySetupIndexMap.Add(BodySetup[i]->BoneName, i-1);
    }
}

void UPhysicsAsset::Serialize(FArchive& Ar)
{
    Ar << BodySetup.Num();
    for (const auto& BodySetup : BodySetup)
    {
        Ar << *BodySetup;
    }
    
    Ar << ConstraintSetup.Num();
    for (const auto& ConstraintSetup : ConstraintSetup)
    {
        Ar << *ConstraintSetup;
    }
}
void UPhysicsAsset::Deserialize(FArchive& Ar)
{
    int BodySetupNum = 0;
    Ar >> BodySetupNum;
    BodySetup.SetNum(BodySetupNum);
    for (UBodySetup*& SetupPtr : BodySetup)
    {
        SetupPtr = FObjectFactory::ConstructObject<UBodySetup>(this);
        Ar >> *SetupPtr;
    }

    int ConstraintSetupNum = 0;
    Ar >> ConstraintSetupNum;
    ConstraintSetup.SetNum(ConstraintSetupNum);
    for (UConstraintSetup*& SetupPtr : ConstraintSetup)
    {
        SetupPtr = FObjectFactory::ConstructObject<UConstraintSetup>(this);
        Ar >> *SetupPtr;
    }
}