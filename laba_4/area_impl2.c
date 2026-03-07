#include "area_calculator.h"

float area_impl2(float a, float b) {
    if (a <= 0 || b <= 0) return 0.0f;
    return 0.5f * a * b;
}