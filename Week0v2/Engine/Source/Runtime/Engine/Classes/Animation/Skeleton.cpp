#include "Skeleton.h"

#include "Skeletal/SkeletalDefine.h"

void USkeleton::Serialize(FArchive& Ar) const 
{
    Ar << *RefSkeletal;
}
void USkeleton::Deserialize(FArchive& Ar)
{
    RefSkeletal = new FRefSkeletal();
    Ar >> *RefSkeletal;
}