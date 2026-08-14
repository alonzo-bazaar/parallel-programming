#include"common.h"

float random_uniform(float min, float max) {
    float random_normalized = (float)random() / (float)INT_MAX;
    return (random_normalized * (max - min)) + min;
}
