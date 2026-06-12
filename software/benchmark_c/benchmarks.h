// benchmarks.h
#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "cache.h"

void simulate_streaming(cache *c);
void simulate_matrix_conv(cache *c);
void simulate_zigzag_access(cache *c);
void simulate_hash_table_noise(cache *c);
void simulate_validation_lru(cache *c);
void simulate_validation_perceptron(cache *c);
void simulate_cyclic_thrashing(cache *c);
void simulate_conditional_pattern(cache *c);
#endif
