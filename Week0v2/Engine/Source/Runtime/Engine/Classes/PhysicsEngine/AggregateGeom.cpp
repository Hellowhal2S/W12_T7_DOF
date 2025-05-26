#include "AggregateGeom.h"
#include "BoxElem.h"
#include "ConvexElem.h"
#include "SphereElem.h"
#include "SphylElem.h"


void FKAggregateGeom::Serialize(FArchive& Ar)
{
    Ar << SphereElems << BoxElems << SphylElems << ConvexElems;
    
}

void FKAggregateGeom::Deserialize(FArchive& Ar)
{
    Ar >> SphereElems >> BoxElems >> SphylElems >> ConvexElems;
}