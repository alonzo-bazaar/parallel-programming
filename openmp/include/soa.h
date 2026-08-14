#ifndef BOID_SOA_H
#define BOID_SOA_H

/**
 * simulation logic for struct of arrays versions of the program
 */

#include<math.h>         // for sqrt
#include"common.h"       // random uniform
#include"config.h"       // simulation configuration global variables

void random_uniform_array(const size_t n_elems, float arr [n_elems],
						  const float min, const float max);

void randomize_boids(const size_t n_boids, config c,
					 float x[restrict n_boids], float y[restrict n_boids],
					 float dx[restrict n_boids], float dy[restrict n_boids]);

void boids_update(const size_t n_boids, config c,
				  const float curr_x[restrict n_boids],
                  const float curr_y[restrict n_boids],
				  const float curr_dx[restrict n_boids],
                  const float curr_dy[restrict n_boids],
				  float next_x[restrict n_boids],
                  float next_y[restrict n_boids],
				  float next_dx[restrict n_boids],
                  float next_dy[restrict n_boids]);

void swapf(float** a, float**b);
void soa_iteration(const size_t num_boids, const config c,
                   float** curr_x,  float** curr_y,
                   float** curr_dx, float** curr_dy,
                   float** next_x,  float** next_y,
                   float** next_dx, float** next_dy);
#endif
