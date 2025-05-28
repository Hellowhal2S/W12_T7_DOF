#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Components/Material/Material.h"
#include "Define.h"

class UBodySetup;

class UStaticMesh : public UObject
{
    DECLARE_CLASS(UStaticMesh, UObject)

public:
    UStaticMesh();
    virtual ~UStaticMesh() override;
    const TArray<FMaterialSlot*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& Out) const;
    OBJ::FStaticMeshRenderData* GetRenderData() const { return staticMeshRenderData; }

    void SetData(OBJ::FStaticMeshRenderData* renderData);
    
    void SetBodySetup(UBodySetup* InBodySetup);
    UBodySetup* GetBodySetup() const { return BodySetup; }
private:
    OBJ::FStaticMeshRenderData* staticMeshRenderData = nullptr;
    UBodySetup* BodySetup;
    TArray<FMaterialSlot*> materials;
};
