#pragma once
#include "core/math/CMatrix.h"

CMatrix operator+(const CMatrix& a, const CMatrix& b);
CMatrix operator-(const CMatrix& a, const CMatrix& b);
CMatrix operator*(const CMatrix& a, const CMatrix& b);

float CosineSimilarity(const CMatrix& a, const CMatrix& b);