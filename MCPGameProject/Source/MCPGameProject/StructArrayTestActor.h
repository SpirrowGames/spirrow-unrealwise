// Test fixture for SpirrowBridge Issue #11 (set_struct_array_property nested struct support).
// Mirrors the issue's repro: USTRUCT containing FIntVector + TObjectPtr<UMaterialInterface>
// inside a TArray on a UCLASS, used to verify nested struct fields are written through
// set_struct_array_property / set_struct_property.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "StructArrayTestActor.generated.h"

USTRUCT(BlueprintType)
struct FStructArrayTestRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntVector Coord = FIntVector::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

UCLASS(Blueprintable)
class MCPGAMEPROJECT_API AStructArrayTestActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    TArray<FStructArrayTestRow> Rows;
};
