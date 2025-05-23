﻿#pragma once
#include "UObject/NameTypes.h"

namespace EAggCollisionShape
{
    enum Type : int
    {
        Sphere,
        Box,
        Sphyl,
        Convex,
        TaperedCapsule,
        LevelSet,
        SkinnedLevelSet,

        Unknown
    };
}

struct FKShapeElem
{
    
    FName Name;

    EAggCollisionShape::Type ShapeType;
};
