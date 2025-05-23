#pragma once
#include "StaticMeshComponent.h"

class UCubeComp : public UStaticMeshComponent
{
    DECLARE_CLASS(UCubeComp, UStaticMeshComponent)

public:
    UCubeComp();
    UCubeComp(UCubeComp& Other);
    virtual ~UCubeComp() override;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
};
