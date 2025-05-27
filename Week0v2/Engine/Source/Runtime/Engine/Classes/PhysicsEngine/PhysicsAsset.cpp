#include "PhysicsAsset.h"
#include "BodySetup.h"

void UPhysicsAsset::UpdateBodySetupIndexMap()
{
    // update BodySetupIndexMap
    BodySetupIndexMap.Empty();

    for(int32 i=0; i < BodySetup.Num(); i++)
    {
        BodySetupIndexMap.Add(BodySetup[i]->BoneName, i);
    }
}

void UPhysicsAsset::Serialize(FArchive& Ar)
{
    Ar << BodySetup.Num();
    for (const auto& BodySetup : BodySetup)
    {
        Ar << *BodySetup;
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
}