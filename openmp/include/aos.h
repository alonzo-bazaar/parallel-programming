#ifndef BOID_AOS_H
#define BOID_AOS_H

/**
 * simulation logic for array of structs versions of the program
 */

#include<math.h>         // for sqrt
#include"common.h"       // random uniform
#include"config.h"       // simulation configuration variables

typedef struct {
	float curr_x; // position read during simulation tick
	float curr_y;
	float next_x; // position written during simulation tick
	float next_y;

	float curr_dx; // velocity read during simulation tick
	float curr_dy;
	float next_dx; // velocity written during simulatin tick
	float next_dy;
} boid;

void randomize_boids(size_t nboids, const config c, boid bs[nboids]);
void boid_compute_position(boid* b);
void boid_compute_velocity(size_t i, size_t nboids,
						   const config* cptr, boid boids[nboids]);
void boid_compute_update(size_t i, size_t nboids,
						 const config* cptr, boid boids[nboids]);
void boid_apply_update(boid* boid);
void boids_update_all(size_t nboids, const config c, boid boids[nboids]);

#endif
