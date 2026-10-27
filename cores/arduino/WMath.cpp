/*
 * WMath.cpp - random() y randomSeed()
 */
#include "Arduino.h"

void randomSeed(unsigned long seed)
{
    if (seed != 0) srandom(seed);
}

long random(long howbig)
{
    if (howbig == 0) return 0;
    return random() % howbig;
}

long random(long howsmall, long howbig)
{
    if (howsmall >= howbig) return howsmall;
    long diff = howbig - howsmall;
    return random(diff) + howsmall;
}
