/*
* Bresenham converted from https://gist.github.com/polyamide/424a9fc8507436883a911e6f1d79d9d3#file-bresenham-js-L1
*
* Edited and converted to Unreal compatible c++ by Fachrurrozy Muhammad
*/

#include "LanGenElevationObject.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Char.h"
#include "Misc/DefaultValueHelper.h"

void ULanGenElevationObject::ResetSeed()
{
    p.Empty();
    p = P_BASE;
    p.Append(P_BASE);
}

void ULanGenElevationObject::Init(int32 in, int x, int y)
{
    lanX = x;
    lanY = y;
    seed = in;
    p.Empty();
    p = P_BASE;
    randomEngine = FRandomStream(seed);
    TArray<int> tempArray = P_BASE;
    Shuffle(tempArray);
    p = tempArray;
    p.Append(tempArray);
}

FVector2D ULanGenElevationObject::RandomizeCoord(float percentageSafeZone)
{
    float half = (1 - percentageSafeZone) / 2;
    int x = randomEngine.RandRange(half * lanX, (percentageSafeZone + half) * lanX),
        y = randomEngine.RandRange(half * lanY, (percentageSafeZone + half) * lanY);
    return FVector2D(
        randomEngine.RandRange(half * lanX, (percentageSafeZone + half) * lanX),
        randomEngine.RandRange(half * lanY, (percentageSafeZone + half) * lanY)
    );
}

FVector2D ULanGenElevationObject::CenterTerrainCoord() { return FVector2D(lanX / 2, lanY / 2); }

TArray<FColor> ULanGenElevationObject::GenerateGraph(
    FVector2D startingPosition, FString rule, FString axiom,
    int ruleLoop, int lineLength, int minAngle,
    int maxAngle, int radius, int peak,
    float skew, int fillDegree, float topBlend,
    int disLoop, float disSmooth, int startHeight
)
{
    TArray<coord> branchRootStack;
    TArray<coord> currentLine;
    coord* currentCoord;
    FString grammar;
    bool isRandomAngle = minAngle != maxAngle;
    int peakIndex = 0;

    init = FColor(startHeight, 0, 0);
    texture.Init(init, lanX * lanY);
    detailTexture.Init(FColor::Black, lanX * lanY);

    // L-System
    RuleSetup(rule);
    grammar = RuleApply(axiom, ruleLoop);

    currentLine.Add(coord(startingPosition.X, startingPosition.Y, 0));

    // create array of target coord
    for (TCHAR i : grammar) {
        currentCoord = &currentLine[currentLine.Num() - 1];
        switch (i) {
        case 'F': Bresenham(currentLine, lineLength); break;
        case 'P': peakIndex = currentLine.Num() - 1; break;
        case 'L': /*main lowest point*/
            if (currentLine.Num() > 2) {
                MidpointDisplacement(currentLine, peak, peakIndex, peak / 2, disLoop, disSmooth);
                GradientSingleMain(currentLine, peak, radius, skew, fillDegree, topBlend, false);
            }
            break;
        case 'D': Bresenham(currentLine, lineLength); break;
        case 'E': /*draw detail*/
            if (currentLine.Num() > 1) {
                for (coord& curCoord : currentLine) curCoord.height = peak * 0.1;
                GradientSingleMain(currentLine, currentCoord->height, 0.1 * radius, 0, 180, 0.5 * topBlend, false, true);
            }
            break;
        case '+': currentCoord->AddTheta(isRandomAngle ? randomEngine.RandRange(minAngle, maxAngle) : minAngle); break;
        case '-': currentCoord->AddTheta(-(isRandomAngle ? randomEngine.RandRange(minAngle, maxAngle) : minAngle)); break;
        case '[': branchRootStack.Add(*currentCoord); break;
        case ']': // run on line ends
            currentLine.Empty();
            currentLine.Add(
                branchRootStack[branchRootStack.Num() - 1]
            );
            branchRootStack.RemoveAt(branchRootStack.Num() - 1);
            break;
        }
    }

    for (int i = 0; i < texture.Num(); ++i) {
        texture[i].R += detailTexture[i].R;
    }

    return texture;

}

TArray<FColor> ULanGenElevationObject::CombineTexture(TArray<FColor> texture1, TArray<FColor> texture2)
{
    if (texture2.Num() != texture1.Num()) return texture1;
    for (int i = 0; i < texture1.Num(); ++i) texture1[i].R = texture1[i].R >= texture2[i].R ? texture1[i].R : texture2[i].R;
    return texture1;
}

void ULanGenElevationObject::RuleSetup(FString in)
{
    /* in = F{[F]F:25,-F:25,+F:25,FF:25} */
    rules.Empty();
    TArray<FString> inRules, tooRule;
    rule inRule;
    FString from, too, to, probString;
    int prob;

    in.ParseIntoArray(inRules, TEXT("}"));
    for (FString i : inRules) {
        i.Split("{", &from, &too);
        inRule.from = from;
        too.ParseIntoArray(tooRule, TEXT(","));
        for (FString j : tooRule) {
            j.Split(":", &to, &probString);
            inRule.to.Add(to);
            FDefaultValueHelper::ParseInt(probString, prob);
            inRule.prob.Add(prob);
        }
        rules.Add(inRule);
        inRule.from.Empty();
        inRule.to.Empty();
        inRule.prob.Empty();
    }
}

FString ULanGenElevationObject::RuleApply(FString axiom, int loop)
{
    FString currentLstring, last;
    int lCount;
    for (int i = 0; i < loop; ++i) {
        lCount = 0;
        last = "";
        currentLstring = "";
        //check for each char in axiom
        for (TCHAR j : axiom) {
            if (lCount < 2) {
                if (j == 'L') ++lCount;
                for (int k = 0; k < rules.Num(); ++k) {
                    if (j == rules[k].from[0]) {
                        currentLstring.Append(RandomizeRule(k, last));
                        break;
                    }
                }
                currentLstring.AppendChar(j);

                if (lCount == 2) { // reset variable after use
                    currentLstring.Append(last);
                    last = "";
                }
            }
            else {
                // rule check
                for (int k = 0; k < rules.Num(); ++k) {
                    if (j == rules[k].from[0]) {
                        currentLstring.Append(RandomizeRule(k, last));
                        break;
                    }
                }
                // if nothing match
                currentLstring.AppendChar(j);

                if (j == 'E') {
                    currentLstring.Append(last);
                    last = "";
                }
            }
        }
        axiom = currentLstring;
        if (axiom.Len() > 1000000000) break; // prevent editor from crashing due string length limit
    }
    return axiom;
}

void ULanGenElevationObject::Shuffle(TArray<int>& inArr) { for (int i = 0; i < inArr.Num(); ++i) inArr.Swap(i, randomEngine.RandRange(0, p.Num() - 1)); }

FString ULanGenElevationObject::RandomizeRule(int ruleIndex, FString& last)
{
    int randomNumber = randomEngine.RandRange(1, 100);
    int currentRequirement = 0;
    FString res;
    bool addLast = false;
    for (int i = 0; i < rules[ruleIndex].to.Num(); ++i) {
        currentRequirement += rules[ruleIndex].prob[i];
        if (randomNumber <= currentRequirement) {
            for (TCHAR j : rules[ruleIndex].to[i]) {
                if (j == ']') addLast = true;

                if (addLast) last.AppendChar(j);
                else res.AppendChar(j);
            }
            return res;
        }
    }
    res = rules[ruleIndex].from;
    return res;
}

void ULanGenElevationObject::Bresenham(TArray<coord>& currentLine, int lineLength)
{
    TArray<coord> res;
    coord currentCoord = currentLine[currentLine.Num() - 1];

    int x0 = currentCoord.x;
    int y0 = currentCoord.y;
    int x = x0 + FMath::Sin(DegreeToRad(currentCoord.theta)) * lineLength;
    int y = y0 + FMath::Cos(DegreeToRad(currentCoord.theta)) * lineLength;

    int dx = FMath::Abs(x - x0);
    int dy = FMath::Abs(y - y0);
    int sx = (x0 < x) ? 1 : -1;
    int sy = (y0 < y) ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (!((x0 == x) && (y0 == y))) {
        e2 = err << 1;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
        currentLine.Add(coord(x0, y0, currentCoord.theta));
    }
}

void ULanGenElevationObject::MidpointDisplacement(TArray<coord>& currentLineCoord, int peak, int peakIndex, int displacement, int loop, float smooth)
{
    TArray<midPoint> oldIndexes, newIndexes;
    midPoint mid;
    int currentDisplacement = displacement,
        randomModifier = 0;
    float modifier = FMath::Pow(2, -smooth), linearM;

    // first half
    if (FMath::Pow(2, loop) > peakIndex) loop = FMath::Log2(peakIndex + 1);

    oldIndexes.Add(midPoint(0, 0));
    oldIndexes.Add(midPoint(peakIndex, peak));

    // fill height between coord
    linearM = LinearM(peak, peakIndex + 1, 1);
    for (int j = 0; j < oldIndexes.Num() - 1; ++j)
        for (int k = oldIndexes[j].index; k < oldIndexes[j + 1].index; ++k) currentLineCoord[k].height = Linear(linearM, k - peakIndex, peak);
    // midpoint
    for (int i = 0; i < loop; ++i) {
        for (int j = 0; j < oldIndexes.Num() - 1; ++j) {
            mid.index = (int)((oldIndexes[j].index + oldIndexes[j + 1].index) / 2);
            newIndexes.Add(oldIndexes[j]);
            if (mid.index != oldIndexes[j].index) {
                currentDisplacement *= modifier;
                mid.height = currentLineCoord[mid.index].height + currentDisplacement * (randomEngine.RandRange(0, 1) == 0 ? -1 : 1);
                newIndexes.Add(mid);
            }
        }
        newIndexes.Add(oldIndexes[oldIndexes.Num() - 1]);
        oldIndexes = newIndexes;
        newIndexes.Empty();
        // fill height between coord
        for (int j = 0; j < oldIndexes.Num() - 1; ++j) {
            for (int k = oldIndexes[j].index; k < oldIndexes[j + 1].index; ++k) {
                currentLineCoord[k].height =
                    (
                        ((float)(k - oldIndexes[j].index) / (oldIndexes[j + 1].index - oldIndexes[j].index)) *
                        (oldIndexes[j + 1].height - oldIndexes[j].height)) +
                    oldIndexes[j].height;
            }
        }
    }

    // second half
    oldIndexes.Empty();
    newIndexes.Empty();
    currentDisplacement = displacement;
    if (FMath::Pow(2, loop) > currentLineCoord.Num() - peakIndex) loop = FMath::Log2(peakIndex + 1);
    oldIndexes.Add(midPoint(peakIndex, peak));
    oldIndexes.Add(midPoint(currentLineCoord.Num() - 1, 0));
    // fill height between coord
    linearM = LinearM(peak, currentLineCoord.Num() - 1 - peakIndex);
    for (int j = 0; j < oldIndexes.Num() - 1; ++j)
        for (int k = oldIndexes[j].index; k < oldIndexes[j + 1].index; ++k) currentLineCoord[k].height = Linear(linearM, k - peakIndex, peak);
    // midpoint
    for (int i = 0; i < loop; ++i) {
        for (int j = 0; j < oldIndexes.Num() - 1; ++j) {
            mid.index = (int)((oldIndexes[j].index + oldIndexes[j + 1].index) / 2);
            newIndexes.Add(oldIndexes[j]);
            if (mid.index != oldIndexes[j].index) {
                currentDisplacement *= modifier;
                mid.height = currentLineCoord[mid.index].height + currentDisplacement * (randomEngine.RandRange(0, 1) == 0 ? -1 : 1);
                newIndexes.Add(mid);
            }
        }
        newIndexes.Add(oldIndexes[oldIndexes.Num() - 1]);
        oldIndexes = newIndexes;
        newIndexes.Empty();
        // fill height between coord
        for (int j = 0; j < oldIndexes.Num() - 1; ++j) {
            for (int k = oldIndexes[j].index; k < oldIndexes[j + 1].index; ++k) {
                currentLineCoord[k].height =
                    (
                        ((float)(k - oldIndexes[j].index) / (oldIndexes[j + 1].index - oldIndexes[j].index)) *
                        (oldIndexes[j + 1].height - oldIndexes[j].height)) +
                    oldIndexes[j].height;
            }
        }
    }
}

void ULanGenElevationObject::GradientSingleMain(TArray<coord>& curLine, int peak, int radius, float skew, int fillDegree, float topBlend, bool calcHeight, bool isAdd)
{
    // main line height
    int mainLineRadius = curLine.Num() / 2,
        mainLineParaRadius = mainLineRadius * 0.75,
        blendHeight, blendOffset;
    float paraA = ParabolaA(peak, mainLineParaRadius);
    float expA = ExponentDecayA(peak, mainLineParaRadius); // use para radius for smoother result
    blendOffset = ParabolaX(paraA, peak, peak / 4);
    blendHeight = (peak / 4) / expA;

    // ridgeline height
    if (calcHeight) {
        for (int i = 0; i < curLine.Num(); ++i) {
            if (i - mainLineRadius < -blendOffset) {
                // left blend
                curLine[i].height = ExponentDecay(expA, blendHeight, i, mainLineRadius - blendOffset, -1);;
            }
            else if (i - mainLineRadius > blendOffset) {
                // right blend
                curLine[i].height = ExponentDecay(expA, blendHeight, i, mainLineRadius + blendOffset);
            }
            else {
                curLine[i].height = Parabola(paraA, peak, i, mainLineRadius);
            }
            // crash *memory usage* prevention
            if (curLine[i].height < 0) curLine[i].height = 0;
        }
    }
    // gradient; peak -> x; radius -> y; y = mx; m = y / x
    int eachRadius;
    float m = (float)radius / peak;

    for (coord curCoord : curLine) {
        eachRadius = m * curCoord.height;
        GradientSingleMainHelper(curCoord, eachRadius, skew, fillDegree, topBlend, isAdd);
    }
}

void ULanGenElevationObject::GradientSingleMainHelper(coord curCoord, int radius, float skew, int fillDegree, float topBlend, bool isAdd)
{
    int leftRadius = radius * ((-skew) + 1),
        rightRadius = radius * (skew + 1),
        curIndex = 0, tempTheta = 0,
        blendLeftHeight = 0, blendRightHeight = 0,
        blendLeftOffset = 0, blendRightOffset = 0,
        leftFunc = 0, rightFunc = 0,
        topBlendLeftStart = 0, topBlendRightStart = 0,
        topBlendPeakOffset = 0, topBlendPeak = 0,
        topBlendOffset = 0, topBlendLeftOffset = 0, topBlendRightOffset = 0,
        xIt, yIt;
    float blendLeftA = 0, blendRightA = 0, curEU = 0, topBlendA = 0,
        leftA, rightA;
    coord startFill = coord(), endFill = coord(),
        a = coord(), b = coord(), c = coord(), d = coord();

    leftA = LinearM(curCoord.height, leftRadius * 0.75),
        blendLeftA = ExponentDecayA(curCoord.height, leftRadius * 0.75),
        blendLeftOffset = LinearX(leftA, curCoord.height, curCoord.height / 4),
        blendLeftHeight = (curCoord.height / 4) / blendLeftA;
    rightA = LinearM(curCoord.height, rightRadius * 0.75),
        blendRightA = ExponentDecayA(curCoord.height, rightRadius * 0.75),
        blendRightOffset = LinearX(rightA, curCoord.height, curCoord.height / 4),
        blendRightHeight = (curCoord.height / 4) / blendRightA;
    topBlendLeftStart = topBlend * leftRadius,
        topBlendRightStart = topBlend * rightRadius,
        topBlendPeakOffset = Linear(leftA, topBlendLeftStart, curCoord.height),
        topBlendPeak = (curCoord.height - topBlendPeakOffset) / 2,
        topBlendOffset = (topBlendLeftStart + topBlendRightStart) / 2,
        topBlendA = ParabolaA(topBlendPeak, topBlendOffset),
        topBlendLeftOffset = topBlendLeftStart - topBlendOffset,
        topBlendRightOffset = topBlendRightStart - topBlendOffset;

    TArray<coord> grad;

    // figuring out where to start loop;
    a.x =
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.AddThetaTemp(180))) * radius) +
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.AddThetaTemp(-90))) * leftRadius);
    a.y =
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.AddThetaTemp(180))) * radius) +
        FMath::RoundToInt((float)FMath::Cos(DegreeToRad(curCoord.AddThetaTemp(-90))) * leftRadius);
    b.x =
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.AddThetaTemp(180))) * radius) +
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.AddThetaTemp(90))) * rightRadius);
    b.y =
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.AddThetaTemp(180))) * radius) +
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.AddThetaTemp(90))) * rightRadius);
    c.x =
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.theta)) * radius) +
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.AddThetaTemp(90))) * rightRadius);
    c.y =
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.theta)) * radius) +
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.AddThetaTemp(90))) * rightRadius);
    d.x =
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.theta)) * radius) +
        FMath::RoundToInt(FMath::Sin(DegreeToRad(curCoord.AddThetaTemp(-90))) * leftRadius);
    d.y =
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.theta)) * radius) +
        FMath::RoundToInt(FMath::Cos(DegreeToRad(curCoord.AddThetaTemp(-90))) * leftRadius);

    // compare Abs value; multply by left n right radius in order for skew to work properly
    startFill.x = (Abs(a.x) > Abs(d.x)) ? a.x : d.x;
    startFill.y = (Abs(a.y * rightRadius) > Abs(b.y * leftRadius)) ? a.y : b.y;
    endFill.x = (Abs(c.x) > Abs(b.x)) ? c.x : b.x;
    endFill.y = (Abs(c.y * leftRadius) > Abs(d.y * rightRadius)) ? c.y : d.y;

    xIt = (endFill.x > startFill.x) ? 1 : -1;
    yIt = (endFill.y > startFill.y) ? 1 : -1;

    grad.Init(coord(), Abs((endFill.x - startFill.x + xIt) * (endFill.y - startFill.y + yIt)));

    for (int x = startFill.x; (xIt == 1) ? x <= endFill.x : x >= endFill.x; x += xIt) {
        for (int y = startFill.y; (yIt == 1) ? y <= endFill.y : y >= endFill.y; y += yIt) {
            curIndex = Abs((x - startFill.x) * (endFill.y - startFill.y)) + Abs(y - startFill.y);
            // fill in x degree left and right only
            tempTheta = RadToDegree(FMath::Atan2(y, x));
            tempTheta = -(tempTheta - 90);
            grad[curIndex].SetTheta(tempTheta); // 0 @ north
            grad[curIndex].AddTheta(-curCoord.theta); // 0 @ center heading
            if ((grad[curIndex].theta > 270 - fillDegree && grad[curIndex].theta < 270 + fillDegree) || (x == 0 && y == 0)) {
                curEU = EuclideanDistance(coord(x, y, 0)); // calculate if condition match only; optimization
                // left
                grad[curIndex].x = x + curCoord.x;
                grad[curIndex].y = y + curCoord.y;

                if (curEU > blendLeftOffset) grad[curIndex].height = ExponentDecay(blendLeftA, blendLeftHeight, curEU, blendLeftOffset);
                else if (curEU < topBlendLeftStart) grad[curIndex].height = Parabola(topBlendA, topBlendPeak + topBlendPeakOffset, Abs(curEU + topBlendLeftOffset));
                else grad[curIndex].height = Linear(leftA, curEU, curCoord.height);
            }
            else if (grad[curIndex].theta > 90 - fillDegree && grad[curIndex].theta < 90 + fillDegree) {
                curEU = EuclideanDistance(coord(x, y, 0)); // calculate if condition match only; optimization
                // right
                grad[curIndex].x = x + curCoord.x;
                grad[curIndex].y = y + curCoord.y;

                if (curEU > blendRightOffset) grad[curIndex].height += ExponentDecay(blendRightA, blendRightHeight, curEU, blendRightOffset);
                else if (curEU < topBlendRightStart) grad[curIndex].height += Parabola(topBlendA, topBlendPeak + topBlendPeakOffset, Abs(curEU + topBlendRightOffset));
                else grad[curIndex].height += Linear(rightA, curEU, curCoord.height);
            }
        }
    }
    isAdd ? DrawDetail(grad) : Draw(grad);
}

float ULanGenElevationObject::EuclideanDistance(coord pointCoord, coord centerCoord)
{
    return FMath::Sqrt(
        FMath::Pow(centerCoord.x - pointCoord.x, 2) + FMath::Pow(centerCoord.y - pointCoord.y, 2)
    );
}

int ULanGenElevationObject::Parabola(float a, float c, float x, float xOffset)
{
    x -= xOffset;
    return (a * x * x) + c;
}

float ULanGenElevationObject::ParabolaA(float c, float xMax, float xOffset)
{
    xMax -= xOffset;
    return (float)-c / (xMax * xMax);
}

float ULanGenElevationObject::ParabolaX(float a, float c, float y) { return FMath::Pow((y - c) / a, 0.5); }

int ULanGenElevationObject::ExponentDecay(float a, float b, float x, float xOffset, int modifier)
{
    x -= xOffset;
    return b * FMath::Pow(a, x * modifier);
}

float ULanGenElevationObject::ExponentDecayA(float b, float xMax, float y, float xOffset)
{
    xMax -= xOffset;
    return FMath::Pow(y / b, 1 / xMax);
}

int ULanGenElevationObject::Linear(float m, float x, float c) { return m * x + c; }

float ULanGenElevationObject::LinearM(float c, float xMax, float modifier) { return modifier * (c / xMax); }

float ULanGenElevationObject::LinearX(float m, float c, float y) { return (y - c) / m; }

void ULanGenElevationObject::Draw(TArray<coord> currentLine, bool overwrite)
{
    /*0 = do not overwerite; 1 = overwrite; 2 = add value*/
    for (coord i : currentLine) {
        if (i.isInRange(lanX, lanY)) {
            /*only draw if current height higher*/
            if (i.height > texture[i.index(lanY)].R - init.R || overwrite) texture[i.index(lanY)].R = i.height + init.R;
        }
    }
}

void ULanGenElevationObject::DrawDetail(TArray<coord> currentLine, bool overwrite)
{
    /*0 = do not overwerite; 1 = overwrite; 2 = add value*/
    for (coord i : currentLine) {
        if (i.isInRange(lanX, lanY)) {
            /*only draw if current height higher*/
            if (i.height > detailTexture[i.index(lanY)].R || overwrite) detailTexture[i.index(lanY)].R = i.height;
        }
    }
}

float ULanGenElevationObject::DegreeToRad(int degree) { return degree * 3.14159265 / 180; }

float ULanGenElevationObject::RadToDegree(float rad) { return rad * 180 / 3.14159265; }

int ULanGenElevationObject::Abs(int in) { return (in < 0) ? in * -1 : in; }