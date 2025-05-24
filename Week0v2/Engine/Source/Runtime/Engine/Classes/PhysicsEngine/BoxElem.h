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
};
