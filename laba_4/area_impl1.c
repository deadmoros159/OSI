#include "area_calculator.h"

float area_impl1(float a, float b) {
    if (a <= 0 || b <= 0) return 0.0f;
    return a * b;
}