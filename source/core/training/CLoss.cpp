#include "core/training/CLoss.h"

#include <algorithm>

float CLoss::ContrastiveLoss(float simPositive, float simNegative, float margin)
{
    return std::max(0.0f, simNegative - simPositive + margin);
}