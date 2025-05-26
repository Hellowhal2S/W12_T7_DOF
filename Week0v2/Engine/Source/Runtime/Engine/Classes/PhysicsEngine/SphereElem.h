#pragma once
#include "ShapeElem.h"
#include "Math/Vector.h"

struct FKSphereElem : public FKShapeElem
{
public:
    FVector Center;

    /** Radius of the sphere */
    float Radius;

    void Serialize(FArchive& Ar) const
    {
        FKShapeElem::Serialize(Ar);
        Ar << Center << Radius;
    }
    void Deserialize(FArchive& Ar)
    {
        FKShapeElem::Deserialize(Ar);
        Ar >> Center >> Radius;
    }
};
