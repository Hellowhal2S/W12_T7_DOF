#pragma once
#include "ShapeElem.h"
#include "Container/Array.h"
#include "Math/Transform.h"

struct FVector;

struct FKConvexElem : public FKShapeElem
{
    TArray<FVector> VertexData;

    TArray<int32> IndexData;

    /** Bounding box of this convex hull. */
    //FBox ElemBox;

private:
    /** Transform of this element */
    FTransform Transform;
};
