#include"soa.h"

// randomize initial state of boids
// (better not to try my luck with parallelizing C random)
void random_uniform_array(const size_t n_elems, float arr [n_elems],
						  const float min, const float max) {
	for(size_t i = 0; i<n_elems; ++i) {
		arr[i] = random_uniform(min, max);
	}
}

// we don't read next_(d)(x|y) at init stage
// we can leave them uninitialized for now
void randomize_boids(const size_t n_boids, const config c,
					 float  x[restrict n_boids], float  y[restrict n_boids],
					 float dx[restrict n_boids], float dy[restrict n_boids]) {
	random_uniform_array(n_boids, x,
					c.turnback_margin, c.window_width - c.turnback_margin);
	random_uniform_array(n_boids, y,
					c.turnback_margin, c.window_height - c.turnback_margin);

	// speeds will be sampled from a square and not from a circle
	// big whoops
	float speed_axis_bound = c.speed_upper_bound / sqrt(2);
	random_uniform_array(n_boids, dx,
						 -speed_axis_bound, speed_axis_bound);
	random_uniform_array(n_boids, dy,
						 -speed_axis_bound, speed_axis_bound);
	
}

void boids_update(const size_t n_boids, const config c,
				  const float  curr_x[restrict n_boids],
                  const float  curr_y[restrict n_boids],
				  const float curr_dx[restrict n_boids],
                  const float curr_dy[restrict n_boids],
				  float  next_x[restrict n_boids],
                  float  next_y[restrict n_boids],
				  float next_dx[restrict n_boids],
                  float next_dy[restrict n_boids]) {
	// compute next positions
#pragma omp parallel
	{
		// compute next velocities
		// data to be computed only after having scanned all neighbours
		// neighbourhood centroid
		float center_x[n_boids];
		float center_y[n_boids];

		// average neighbourhood speed
		float avg_neigh_dx[n_boids];
		float avg_neigh_dy[n_boids];

		int n_neighbours[n_boids];

		// replusive force to apply to boid to avoid it crashing on its neighbours
		float repulse_x[n_boids];
		float repulse_y[n_boids];

#pragma omp for
		for(size_t i = 0; i<n_boids; ++i) {
			center_x[i] = 0.0f; 
			center_y[i] = 0.0f; 
			avg_neigh_dx[i] = 0.0f; 
			avg_neigh_dy[i] = 0.0f; 
			n_neighbours[i] = 0; 
			repulse_x[i] = 0.0f; 
			repulse_y[i] = 0.0f;
		}

		// data collection loop
#pragma omp for
		for(size_t i = 0; i<n_boids-1; ++i) {
			for(size_t j = i+1; j<n_boids; ++j) {
				float dx = curr_x[i] - curr_x[j];
				float dy = curr_y[i] - curr_y[j];
				float dist = sqrt((dx*dx) + (dy*dy));

				if(dist < c.too_close_radius) {
					repulse_x[i] += dx;
					repulse_y[i] += dy;

					repulse_x[j] -= dx;
					repulse_y[j] -= dy;
				}

				if(dist < c.neighbour_radius) {
					avg_neigh_dx[i] += curr_dx[j];
					avg_neigh_dy[i] += curr_dy[j];

					avg_neigh_dx[j] += curr_dx[i];
					avg_neigh_dy[j] += curr_dy[i];

					center_x[i] += curr_x[j];
					center_y[i] += curr_y[j];

					center_x[j] += curr_x[i];
					center_y[j] += curr_y[i];

					n_neighbours[i]++;
					n_neighbours[j]++;
				}
			}

			if(n_neighbours[i] > 0) {
				center_x[i]/=n_neighbours[i];
				center_y[i]/=n_neighbours[i];
				avg_neigh_dx[i]/=n_neighbours[i];
				avg_neigh_dy[i]/=n_neighbours[i];
			}
		}

		// position update
		// position the boids will have in the next update
#pragma omp for
		for(size_t i = 0; i<n_boids; ++i) {
			next_x[i] = curr_x[i] + curr_dx[i];
			next_y[i] = curr_y[i] + curr_dy[i];
		}

		// speed update
		// the following operations will all act on the boid's speed alone
		// "make boid do this" here means
		// "update boid speed in a way that will make it go towards doing this"

#pragma omp for
		for(size_t i = 0; i<n_boids; ++i) {
			// velocity starting point
			next_dx[i] = curr_dx[i];
			next_dy[i] = curr_dy[i];

			// make boid center itself with respect to its neighbours
			if(n_neighbours[i] > 0) {
				next_dx[i] += (center_x[i] - curr_x[i]) * c.centering_factor;
				next_dy[i] += (center_y[i] - curr_y[i]) * c.centering_factor;
			}

			// make boid avoid collisions
			next_dx[i] += repulse_x[i] * c.dont_slam_factor;
			next_dy[i] += repulse_y[i] * c.dont_slam_factor;

			// make boid go at the same speed as its neighbours
			if(n_neighbours[i] > 0) {
				next_dx[i] += (avg_neigh_dx[i] - curr_dx[i])\
					* c.speed_match_factor;
				next_dy[i] += (avg_neigh_dy[i] - curr_dy[i])\
					* c.speed_match_factor;
			}

			// make boid stay within window bounds
			if(curr_x[i] > c.window_width - c.turnback_margin)
				next_dx[i] -= c.turnback_factor;
			if(curr_y[i] > c.window_height - c.turnback_margin)
				next_dy[i] -= c.turnback_factor;

			if(curr_x[i] < c.turnback_margin)
				next_dx[i] += c.turnback_factor;
			if(curr_y[i] < c.turnback_margin)
				next_dy[i] += c.turnback_factor;

			// clamp boid acceleration within an acceptable range
			float ax = next_dx[i] - curr_dx[i];
			float ay = next_dy[i] - curr_dy[i];
			float a = sqrt(ax*ax + ay*ay);
			if(a > c.acceleration_upper_bound) {
				ax*=c.acceleration_upper_bound/a;
				ay*=c.acceleration_upper_bound/a;

				next_dx[i] = curr_dx[i]+ax;
				next_dy[i] = curr_dy[i]+ay;
			}

			// clamp boid speed within an acceptable range
			float v = sqrt(next_dx[i]*next_dx[i] + 
						   next_dy[i]*next_dy[i]);
			if(v > c.speed_upper_bound) {
				next_dx[i] *= c.speed_upper_bound/v;
				next_dy[i] *= c.speed_upper_bound/v;
			}

			if(v < c.speed_lower_bound) {
				next_dx[i] *= c.speed_lower_bound/v;
				next_dy[i] *= c.speed_lower_bound/v;
			}
		}
	}
}

void swapf(float** a, float**b) {
    float* tmp = *a;
    *a = *b;
    *b = tmp;
}

void soa_iteration(const size_t num_boids, const config c,
                   float** curr_x,  float** curr_y,
                   float** curr_dx, float** curr_dy,
                   float** next_x,  float** next_y,
                   float** next_dx, float** next_dy) {

	boids_update(num_boids, c,
				 *curr_x, *curr_y, *curr_dx, *curr_dy,
				 *next_x, *next_y, *next_dx, *next_dy);

    swapf(curr_x,  next_x);
    swapf(curr_y,  next_y);
    swapf(curr_dx, next_dx);
    swapf(curr_dy, next_dy);
}
