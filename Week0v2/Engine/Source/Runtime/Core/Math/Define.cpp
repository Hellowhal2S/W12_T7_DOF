#include "Define.h"

#include "Components/Material/Material.h"
#include "Runtime/Serialization/Archive.h"
#include "UObject/ObjectFactory.h"

void FMaterialSubset::Serialize(FArchive& Ar) const
{
    Ar << IndexStart << IndexCount << MaterialIndex << MaterialName;
}

void FMaterialSubset::Deserialize(FArchive& Ar)
{
    Ar >> IndexStart >> IndexCount >> MaterialIndex >> MaterialName;
}

void FMaterialSlot::Serialize(FArchive& ar) const
{
    ar << *Material << MaterialSlotName;
}

void FMaterialSlot::Deserialize(FArchive& ar)
{
    Material = FObjectFactory::ConstructObject<UMaterial>(nullptr);
    ar >> *Material >> MaterialSlotName;
}

void FObjMaterialInfo::Serialize(FArchive& Ar) const
{
    Ar << MTLName << bHasTexture << bTransparent;
    Ar << Diffuse << Specular << Ambient << Emissive
        << SpecularScalar << DensityScalar << TransparencyScalar << IlluminanceModel;
    Ar << DiffuseTextureName << DiffuseTexturePath
        << AmbientTextureName << AmbientTexturePath
        << SpecularTextureName << SpecularTexturePath
        << BumpTextureName << BumpTexturePath
        << AlphaTextureName << AlphaTexturePath
        << NormalTextureName << NormalTexturePath;
    Ar << NormalScale;
}

void FObjMaterialInfo::Deserialize(FArchive& Ar)
{
    Ar >> MTLName >> bHasTexture >> bTransparent;
    Ar >> Diffuse >> Specular >> Ambient >> Emissive
        >> SpecularScalar >> DensityScalar >> TransparencyScalar >> IlluminanceModel;
    Ar >> DiffuseTextureName >> DiffuseTexturePath
        >> AmbientTextureName >> AmbientTexturePath
        >> SpecularTextureName >> SpecularTexturePath
        >> BumpTextureName >> BumpTexturePath
        >> AlphaTextureName >> AlphaTexturePath
        >> NormalTextureName >> NormalTexturePath;
    Ar >> NormalScale;
}