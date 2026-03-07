#include <math.h>
#include "e_calculator.h"

float e_impl1(int x) {
    if (x <= 0) return 0.0f;
    return powf(1.0f + 1.0f/x, x);
}