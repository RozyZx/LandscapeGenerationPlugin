/*
* PERLIN NOISE FUNCTION:
* THE ORIGINAL JAVA IMPLEMENTATION IS COPYRIGHT 2002 KEN PERLIN
* Edited and converted to C++ by Paul Silisteanu
* source: https://github.com/sol-prog/Perlin_Noise/blob/master/PerlinNoise.cpp
*
* SIMPLEX NOISE FUNCTION:
* Copyright (c) 2014-2018 Sebastien Rombauts (sebastien.rombauts@gmail.com)
* This C++ implementation is based on the speed-improved Java version 2012-03-09
* by Stefan Gustavson (original Java source code in the public domain).
* http://webstaff.itn.liu.se/~stegu/simplexnoise/SimplexNoise.java:
* - Based on example code by Stefan Gustavson (stegu@itn.liu.se).
* - Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
* - Better rank ordering method by Stefan Gustavson in 2012.
* This implementation is "Simplex Noise" as presented by
* Ken Perlin at a relatively obscure and not often cited course
* session "Real-Time Shading" at Siggraph 2001 (before real
* time shading actually took on), under the title "hardware noise".
* The 3D function is numerically equivalent to his Java reference
* code available in the PDF course notes, although I re-implemented
* it from scratch to get more readable code. The 1D, 2D and 4D cases
* were implemented from scratch by me from Ken Perlin's text.
* Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
* or copy at http://opensource.org/licenses/MIT)
* source: https://github.com/SRombauts/SimplexNoise/blob/master/src/SimplexNoise.cpp
*
* Edited and converted to Unreal compatible c++ by Fachrurrozy Muhammad
*/

#include "LanGenNoiseObject.h"

void ULanGenNoiseObject::ResetSeed()
{
    p.Empty();
    p = P_BASE;
    p.Append(P_BASE);
}

void ULanGenNoiseObject::InitSeed(int32 in)
{
    seed = in;
    p.Empty();
    p = P_BASE;
    randomEngine = FRandomStream(seed);
    TArray<int> tempArray = P_BASE;
    Shuffle(&tempArray);
    p = tempArray;
    p.Append(tempArray);
}

float ULanGenNoiseObject::PerlinNoise3D(FVector location, int n, float lacunarity, float persistence, float in)
{
    // change frequency by changing x y z input value
    float multiplier = FMath::Pow(lacunarity, n);
    float x = location.X * multiplier;
    float y = location.Y * multiplier;
    float z = location.Z * multiplier;

    // return value between -1.0 to 1.0
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;

    // Find relative x, y,z of point in cube
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    // Compute fade curves for each of x, y, z
    float u = Fade(x);
    float v = Fade(y);
    float w = Fade(z);

    // Hash coordinates of the 8 cube corners
    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    // Add blended results from 8 corners of cube
    float res =
        Lerp(w,
            Lerp(v,
                Lerp(u,
                    Grad(p[AA], x, y, z),
                    Grad(p[BA], x - 1, y, z)),
                Lerp(u,
                    Grad(p[AB], x, y - 1, z),
                    Grad(p[BB], x - 1, y - 1, z))),
            Lerp(v,
                Lerp(u,
                    Grad(p[AA + 1], x, y, z - 1),
                    Grad(p[BA + 1], x - 1, y, z - 1)),
                Lerp(u,
                    Grad(p[AB + 1], x, y - 1, z - 1),
                    Grad(p[BB + 1], x - 1, y - 1, z - 1))));

    // change amplitude by scaling down
    res *= FMath::Pow(persistence, n);

    // blend value
    in += res;

    return in;
}

float ULanGenNoiseObject::SimplexNoise3D(FVector location, int n, float lacunarity, float persistence, float in)
{
    // change frequency by changing x y z input value
    float multiplier = FMath::Pow(lacunarity, n);
    float x = location.X * multiplier;
    float y = location.Y * multiplier;
    float z = location.Z * multiplier;

    float n0, n1, n2, n3; // Noise contributions from the four corners

    // Skewing/Unskewing factors for 3D
    static const float F3 = 1.0f / 3.0f;
    static const float G3 = 1.0f / 6.0f;

    // Skew the input space to determine which simplex cell we're in
    float s = (x + y + z) * F3; // Very nice and simple skew factor for 3D
    int i = floor(x + s);
    int j = floor(y + s);
    int k = floor(z + s);
    float t = (i + j + k) * G3;
    float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0; // The x,y,z distances from the cell origin
    float y0 = y - Y0;
    float z0 = z - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
    int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords
    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; // X Y Z order
        }
        else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; // X Z Y order
        }
        else {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; // Z X Y order
        }
    }
    else { // x0<y0
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; // Z Y X order
        }
        else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; // Y Z X order
        }
        else {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; // Y X Z order
        }
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
    // c = 1/6.
    float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    // Work out the hashed gradient indices of the four simplex corners
    int gi0 = Hash(i + Hash(j + Hash(k)));
    int gi1 = Hash(i + i1 + Hash(j + j1 + Hash(k + k1)));
    int gi2 = Hash(i + i2 + Hash(j + j2 + Hash(k + k2)));
    int gi3 = Hash(i + 1 + Hash(j + 1 + Hash(k + 1)));

    // Calculate the contribution from the four corners
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0) n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * Grad(gi0, x0, y0, z0);
    }
    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0) n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * Grad(gi1, x1, y1, z1);
    }
    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0) n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * Grad(gi2, x2, y2, z2);
    }
    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0) n3 = 0.0;
    else {
        t3 *= t3;
        n3 = t3 * t3 * Grad(gi3, x3, y3, z3);
    }
    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    float res = 32.0f * (n0 + n1 + n2 + n3);

    //scale amplitude
    res *= FMath::Pow(persistence, n);

    //blend
    in += res;

    return in;
}

float ULanGenNoiseObject::Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

float ULanGenNoiseObject::Lerp(float t, float a, float b) { return a + t * (b - a); }

float ULanGenNoiseObject::Grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    // Convert lower 4 bits of hash into 12 gradient directions
    float u = h < 8 ? x : y,
        v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

void ULanGenNoiseObject::Shuffle(TArray<int>* inArr) { for (int i = 0; i < inArr->Num(); ++i) inArr->Swap(i, randomEngine.RandRange(0, inArr->Num() - 1)); }

uint8_t ULanGenNoiseObject::Hash(int32_t i) { return p[static_cast<uint8_t>(i)]; }