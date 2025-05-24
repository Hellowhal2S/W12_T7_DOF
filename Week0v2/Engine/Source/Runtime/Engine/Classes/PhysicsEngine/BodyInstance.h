#pragma once
#include "BodyInstanceCore.h"

class UPrimitiveComponent;

struct FBodyInstance : public FBodyInstanceCore
{
    UPrimitiveComponent* OwnerComponent;    
};
