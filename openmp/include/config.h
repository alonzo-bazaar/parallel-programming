#ifndef BOID_VARS_H
#define BOID_VARS_H

// some preprocessor abuse was performed to avoid excessive duplication
// between graphical and benchmark versions of the program

typedef struct {
	int num_threads;
	int num_boids;

	int window_width; // dimensions of raylib window (or simulation area)
	int window_height;
#ifdef BOID_GRAPHICS
	int window_fps;
#endif

#ifdef BOID_BENCHMARK
	int num_warmup_iterations; // number of warmup iterations before measuring
	int num_iterations;		   // number of iterations in benchmark
	char* output_file;		   // file where benchmark results will be written
#endif

	float turnback_margin; // distance from window border at which boids
	                       // starts turning back
	float turnback_factor; // strength of turning back

	float neighbour_radius;   // min distance for two boids to be neighbours
	float centering_factor;   // strength of neighbour centering
	float speed_match_factor; // strength of neighbour speed matching

	float too_close_radius; // distance at which boids start avoiding each other
	float dont_slam_factor; // strength of collision avoidance potential

	float speed_upper_bound; // maximum possible boid speed
	float speed_lower_bound; // minimum possible boid speed

	float acceleration_upper_bound; // maximum possible acceleration in one tick
} config;

void config_set_default(config* c);

#endif
