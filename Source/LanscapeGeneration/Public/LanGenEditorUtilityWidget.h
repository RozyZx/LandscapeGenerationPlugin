// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Math/Color.h"
#include "LanGenEditorUtilityWidget.generated.h"

/**
 * 
 */
UCLASS()
class LANSCAPEGENERATION_API ULanGenEditorUtilityWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
protected:
	uint8 NoiseToColor(float in);

public:
	UFUNCTION(BlueprintCallable)
		static void ConLog(FString text);
	UFUNCTION(BlueprintCallable)
		static void ScrLog(FString text);
	UFUNCTION(BlueprintCallable, Category = "Landscape Generation")
		UTexture2D* GenerateTexture(int x, int y, int tileX = 512, int tileY = 512, int generateParam = 0);
	UFUNCTION(BlueprintCallable, Category = "Landscape Generation")
		uint8 MapTo8Bit(float in, float min = -1.0, float max = 1.0);
	UFUNCTION(BlueprintCallable, Category = "Landscape Generation")
		int MapFloatToInt(float in, float inMin, float inMax, int min = -100, int max = 100);
	UFUNCTION(BlueprintCallable, Category = "Landscape Generation")
		float RandomFloat();
	UFUNCTION(BlueprintCallable, Category = "Landscape Generation")
		int32 Randomize() { return FMath::RandRange((int32)-1000000000, (int32)1000000000); }

	UFUNCTION(BlueprintImplementableEvent, Category = "Landscape Generation")
		uint8 GenerateFunction(const FVector2D location, const int value);
	UFUNCTION(BlueprintImplementableEvent, Category = "Landscape Generation")
		TArray<FColor> GenerateElevationMap(const int x, const int y);
};
