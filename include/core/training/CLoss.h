#pragma once

#include "core/math/CMatrix.h"

class CLoss
{
public:
    static float ContrastiveLoss(float simPositive, float simNegative, float margin = 0.2f);
};