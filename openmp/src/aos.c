#include "aos.h"

// randomize initial state of boids
// (better not to try my luck with parallelizing C random)
void randomize_boids(size_t nboids, const config c, boid bs[nboids]) {
    float speed_comp_limit = c.speed_upper_bound/sqrt(2.0);

    for(size_t i = 0; i< nboids; ++i) {
        bs[i].curr_x = random_uniform(c.turnback_margin,
									 c.window_width - c.turnback_margin);
        bs[i].curr_y = random_uniform(c.turnback_margin,
									 c.window_height - c.turnback_margin);
        bs[i].next_x = bs[i].curr_x;
		bs[i].next_y = bs[i].curr_x;

        bs[i].curr_dx = random_uniform(-speed_comp_limit, speed_comp_limit);
        bs[i].curr_dy = random_uniform(-speed_comp_limit, speed_comp_limit);
        bs[i].next_dx = bs[i].curr_dx;
        bs[i].next_dy = bs[i].curr_dx;
    }
}

// compute position that boid will have during next simulation tick 

// reads:  curr_x, curr_y, curr_dx, curr_dy
// writes: next_x, next_y
void boid_compute_position(boid* b) {
	b->next_x = b->curr_x + b->curr_dx;
	b->next_y = b->curr_y + b->curr_dy;
}

// update a single boids's speed within a boid vector
// I apologize for this function being rather gigantic
// I wanted to merge as many loops as possible
// to have an easier time experimenting with parallelization strats

// reads:  curr_x, curr_y, curr_dx, curr_dy
// writes: next_dx, next_dy
void boid_compute_velocity(size_t i, size_t nboids,
						   const config* cptr, boid boids[nboids]) {
	const config c = *cptr;

	// data to be computed only after having scanned all neighbours
	// neighbourhood centroid
	float center_x = 0.0;
	float center_y = 0.0;

	// average neighbourhood speed
	float avg_neigh_dx = 0.0;
	float avg_neigh_dy = 0.0;

	int n_neighbours = 0;

	// replusive force to apply to boid to avoid it crashing on its neighbours
	float repulse_x = 0.0;
	float repulse_y = 0.0;

	// data collection
	for(size_t j = 0; j<nboids; ++j) {
		if(j!=i) {
			float dx = boids[i].curr_x - boids[j].curr_x;
			float dy = boids[i].curr_y - boids[j].curr_y;
			float dist = sqrt((dx*dx) + (dy*dy));

			if(dist < c.too_close_radius) {
				repulse_x += dx;
				repulse_y += dy;
			}

			if(dist < c.neighbour_radius) {
				avg_neigh_dx += boids[j].curr_dx;
				avg_neigh_dy += boids[j].curr_dy;

				center_x += boids[j].curr_x;
				center_y += boids[j].curr_y;

				n_neighbours++;
			}
		}
	}

	if(n_neighbours > 0) {
		center_x/=n_neighbours;
		center_y/=n_neighbours;
		avg_neigh_dx/=n_neighbours;
		avg_neigh_dy/=n_neighbours;
	}

	// speed update
	// the following operations will all act on the boid's speed alone
	// "make boid do this" here means
	// "update boid speed in a way that will make it go towards doing this"

	// velocity starting point
	boids[i].next_dx = boids[i].curr_dx;
	boids[i].next_dy = boids[i].curr_dy;

	// make boid center itself with respect to its neighbours
	if(n_neighbours > 0) {
		boids[i].next_dx += (center_x - boids[i].curr_x)\
			* c.centering_factor;
		boids[i].next_dy += (center_y - boids[i].curr_y)\
			* c.centering_factor;
	}

	// make boid avoid collisions
	boids[i].next_dx += repulse_x * c.dont_slam_factor;
	boids[i].next_dy += repulse_y * c.dont_slam_factor;

	// make boid go at the same speed as its neighbours
	if(n_neighbours > 0) {
		boids[i].next_dx += (avg_neigh_dx - boids[i].curr_dx)\
			* c.speed_match_factor;
		boids[i].next_dy += (avg_neigh_dy - boids[i].curr_dy)\
			* c.speed_match_factor;
	}

	// make boid stay within window bounds
	if(boids[i].curr_x > c.window_width - c.turnback_margin)
		boids[i].next_dx -= c.turnback_factor;
	if(boids[i].curr_y > c.window_height - c.turnback_margin)
		boids[i].next_dy -= c.turnback_factor;

	if(boids[i].curr_x < c.turnback_margin)
		boids[i].next_dx += c.turnback_factor;
	if(boids[i].curr_y < c.turnback_margin)
		boids[i].next_dy += c.turnback_factor;

	// clamp boid acceleration within an acceptable range
	float ax = boids[i].next_dx - boids[i].curr_dx;
	float ay = boids[i].next_dy - boids[i].curr_dy;
	float a = sqrt(ax*ax + ay*ay);
	if(a > c.acceleration_upper_bound) {
		ax*=c.acceleration_upper_bound/a;
		ay*=c.acceleration_upper_bound/a;

		boids[i].next_dx = boids[i].curr_dx+ax;
		boids[i].next_dy = boids[i].curr_dy+ay;
	}

 	// clamp boid speed within an acceptable range
 	float v = sqrt(boids[i].next_dx*boids[i].next_dx + 
 				   boids[i].next_dy*boids[i].next_dy);
 	if(v > c.speed_upper_bound) {
		boids[i].next_dx *= c.speed_upper_bound/v;
		boids[i].next_dy *= c.speed_upper_bound/v;
	}

	if(v < c.speed_lower_bound) {
		boids[i].next_dx *= c.speed_lower_bound/v;
 		boids[i].next_dy *= c.speed_lower_bound/v;
 	}
}
 
// find boids[i].next_... given boids[i].curr_...
void boid_compute_update(size_t i, const size_t nboids,
						 const config* cptr, boid boids[nboids]) {
 	boid_compute_position(boids + i);
 	boid_compute_velocity(i, nboids, cptr, boids);
}
 
// set the boids[i].new... you just found
void boid_apply_update(boid* boid) {
 	boid->curr_x = boid->next_x;
 	boid->curr_y = boid->next_y;
 	boid->curr_dx = boid->next_dx;
 	boid->curr_dy = boid->next_dy;
}
 
void boids_update_all(const size_t nboids, const config c,
					  boid boids[nboids]) {
 	// coarse (boid level) parallelism, neighbour scan is not parallel
#pragma omp parallel
 	{
#pragma omp for
		for(size_t i = 0; i<c.num_boids; ++i)
			boid_compute_update(i, nboids, &c, boids);
		// we need this barrier

#pragma omp for
		for(size_t i = 0; i<c.num_boids; ++i)
			boid_apply_update(boids + i);
 	}
}
