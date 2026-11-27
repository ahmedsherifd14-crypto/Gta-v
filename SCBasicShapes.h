#pragma once

#include "CoreMinimal.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

/**
 * Engine-provided placeholder assets (no project content required).
 * IMPORTANT: ConstructorHelpers may only be used while a UObject constructor is running,
 * so call these ONLY from constructors and cache the results in UPROPERTY members.
 */
namespace SCShapes
{
    inline UStaticMesh* Cube()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(TEXT("/Engine/BasicShapes/Cube.Cube"));
        return Finder.Object;
    }

    inline UStaticMesh* Cylinder()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        return Finder.Object;
    }

    inline UStaticMesh* Sphere()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        return Finder.Object;
    }

    /** Material that exposes a "Color" vector parameter (tinted via dynamic material instances). */
    inline UMaterialInterface* BasicMaterial()
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> Finder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        return Finder.Object;
    }
}
