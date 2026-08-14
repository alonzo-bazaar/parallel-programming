#include"config.h"

void config_set_default(config* c) {
	c->num_threads = 8;
	c->num_boids = 100;

	c->window_width = 1000;
	c->window_height = 1000;
#ifdef BOID_GRAPHICS
	c->window_fps = 60;
#endif

#ifdef BOID_BENCHMARK
	c->num_warmup_iterations = 100;
	c->num_iterations = 2000;
	c->output_file = "./results/benchmark.csv";
#endif

	c->turnback_margin = 100.0;
	c->turnback_factor = 1.0;

	c->neighbour_radius = 75.0;
	c->centering_factor = 0.005;
	c->speed_match_factor = 0.05;

	c->too_close_radius = 20.0;
	c->dont_slam_factor = 0.05;

	c->speed_upper_bound = 50.00;
	c->speed_lower_bound = 5.00;

	c->acceleration_upper_bound = 2.0;
}
