#include "e_calculator.h"

float e_impl2(int x) {
    if (x < 0) return 0.0f;
    
    float result = 0.0f;
    float factorial = 1.0f;
    
    for (int n = 0; n <= x; n++) {
        if (n > 0) {
            factorial *= n;
        }
        result += 1.0f / factorial;
    }
    
    return result;
}