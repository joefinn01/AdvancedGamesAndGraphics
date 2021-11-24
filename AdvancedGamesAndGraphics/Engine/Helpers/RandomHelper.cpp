#include "RandomHelper.h"

#include <random>

float RandomHelper::RandomFloat(float fLowerLimit, float fUpperLimit)
{
	return ((float)rand() / RAND_MAX) * (fUpperLimit - fLowerLimit) + fLowerLimit;
}
