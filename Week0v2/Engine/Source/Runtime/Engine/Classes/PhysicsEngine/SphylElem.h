#pragma once
#include "ShapeElem.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"

struct FKSphylElem : public FKShapeElem
{
public:
    /** Position of the capsule's origin */
    FVector Center;

    /** Rotation of the capsule */
    FRotator Rotation;

    /** Radius of the capsule */
    float Radius;

    /** This is of line-segment ie. add Radius to both ends to find total length. */
    float Length;
};
