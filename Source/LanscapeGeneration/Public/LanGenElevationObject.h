// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Math/Color.h"
#include "LanGenElevationObject.generated.h"

struct rule {
	FString from;
	TArray<FString> to;
	TArray<int> prob;
};

struct coord {
	int x, y, theta, height;
	coord() { x = 0, y = 0, theta = 0, height = 0; }
	coord(int X, int Y, int THETA) { x = X, y = Y, theta = THETA, height = 0; }
	void SetTheta(int value) { theta = Mod(value); }
	void AddTheta(int value) { theta = Mod(theta + value); }
	int AddThetaTemp(int value) { return (theta + value) % 360; }
	int index(int yLen) { return x * yLen + y; }
	bool isInRange(int xLen, int yLen) { return x >= 0 && y >= 0 && x < xLen&& y < yLen; }
	static int Mod(int in) {
		int tempInt = in % 360;
		if (tempInt < 0) tempInt += 360;
		return tempInt;
	}
};

struct midPoint {
	int index, height;
	midPoint() { index = 0, height = 0; }
	midPoint(int x, int y) { index = x, height = y; }
};

UCLASS(BlueprintType)
class LANSCAPEGENERATION_API ULanGenElevationObject : public UObject
{
	GENERATED_BODY()
public:
	int32 seed;
private:
	FRandomStream randomEngine;
	TArray<rule> rules;
	TArray<int> p;
	TArray<FColor> texture, detailTexture;
	const TArray<int> P_BASE = {
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
		8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
		35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,
		134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
		55,46,245,40,244,102,143,54, 65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
		18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,
		250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
		189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,
		43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,
		97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,
		107,49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
		138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};
	FColor init;
	int lanX, lanY;
public:
	UFUNCTION(BlueprintCallable, Category = "LanGen Elevation")
		void ResetSeed();
	UFUNCTION(BlueprintCallable, Category = "LanGen Elevation")
		void Init(int32 in, int x, int y);
	UFUNCTION(BlueprintCallable, Category = "LanGen Elevation")
		FVector2D RandomizeCoord(float percentageSafeZone);
	UFUNCTION(BlueprintCallable, Category = "LanGen Elevation")
		FVector2D CenterTerrainCoord();
	UFUNCTION(BlueprintCallable, Category = "LanGen Elevation")
		TArray<FColor> GenerateGraph(
			FVector2D startingPosition, FString rule, FString axiom,
			int ruleLoop, int lineLength = 3, int minAngle = 30,
			int maxAngle = 30, int radius = 50, int peak = 50,
			float skew = 0, int fillDegree = 90, float topBlend = 0.1,
			int disLoop = 5, float disSmooth = 1.1, int startHeight = 50
		);
	UFUNCTION(BlueprintCallable, Category = "LanGen Elevation")
		TArray<FColor> CombineTexture(TArray<FColor> texture1, TArray<FColor> texture2);
private:
	void RuleSetup(FString rule);
	FString RuleApply(FString axiom, int loop);
	void Shuffle(TArray<int>& inArr);
	FString RandomizeRule(int ruleIndex, FString& last);
	void Bresenham(TArray<coord>& currentLine, int lineLength);
	void MidpointDisplacement(TArray<coord>& currentLine, int peak, int peakIndex, int displacement, int loop, float smooth = 1.1);

	void GradientSingleMain(TArray<coord>& curLine, int peak, int radius, float skew, int fillDegree, float topBlend, bool calcHeight = true, bool isAdd = false);
	void GradientSingleMainHelper(coord curCoord, int radius, float skew, int fillDegree, float topBlend, bool isAdd = false);

	float EuclideanDistance(coord pointCoord, coord centerCoord = coord());
	int Parabola(float a, float c, float x, float xOffset = 0);
	float ParabolaA(float c, float xMax, float xOffset = 0);
	float ParabolaX(float a, float c, float y);
	int ExponentDecay(float a, float b, float x, float xOffset = 0, int modifier = 1);
	float ExponentDecayA(float b, float xMax, float y = 0.1, float xOffset = 0);
	int Linear(float m, float x, float c);
	float LinearM(float c, float xMax, float modifier = -1);
	float LinearX(float m, float c, float y);

	void Draw(TArray<coord> currentLine, bool overwrite = false);
	void DrawDetail(TArray<coord> currentLine, bool overwrite = false);
	static float DegreeToRad(int degree);
	static float RadToDegree(float rad);
	static int Abs(int in);
};
