#include "ShapeElem.h"

#include "Container/String.h"

void FKShapeElem::Serialize(FArchive& Ar) const
{
    Ar << Name.ToString() << bEnableCollision << static_cast<int>(ShapeType);
    
}

void FKShapeElem::Deserialize(FArchive& Ar)
{
    int temp;
    Ar >> Name >> bEnableCollision >> temp;
    ShapeType = static_cast<EAggCollisionShape::Type>(temp);
}