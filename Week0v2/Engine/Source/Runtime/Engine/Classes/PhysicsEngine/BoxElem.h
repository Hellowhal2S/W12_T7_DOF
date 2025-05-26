#pragma once
#include "ShapeElem.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"

struct FKBoxElem : public FKShapeElem
{
    FVector Center;

    /** Rotation of the box */
    FRotator Rotation;

    /** Extent of the box along the y-axis */
    float X;

    /** Extent of the box along the y-axis */
    float Y;

    /** Extent of the box along the z-axis */
    float Z;
    void Serialize(FArchive& Ar) const 
    {
        FKShapeElem::Serialize(Ar);
        Ar << Center << Rotation << X << Y << Z;
    }
    void Deserialize(FArchive& Ar)
    {
        FKShapeElem::Deserialize(Ar);
        Ar >> Center >> Rotation >> X >> Y >> Z;
    }
};
