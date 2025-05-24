#pragma once
#include "ShapeElem.h"
#include "Math/Vector.h"

struct FKSphereElem : public FKShapeElem
{
public:
    FVector Center;

    /** Radius of the sphere */
    float Radius;
};
