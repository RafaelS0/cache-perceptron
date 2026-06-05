// benchmarks.h
#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "cache.h"

void simulate_streaming(cache *c);
void simulate_matrix_conv(cache *c);
void simulate_validation_lru(cache *c);
#endif