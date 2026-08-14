#include<omp.h>              // set num threads
#include"args.h"             // some global variables and argv parsing
#include"benchmark-common.h" // timing and report generation functions

#include"soa.h"

int main(int argc, char** argv) {
	// sets some of global configuration variables used throughout the program
	config c;
	config_set_default(&c);
	handle_argv_vars(argc, argv, &c);
    omp_set_num_threads(c.num_threads);

	float curr_x[c.num_boids];  float curr_y[c.num_boids];
	float curr_dx[c.num_boids]; float curr_dy[c.num_boids];
	float next_x[c.num_boids];  float next_y[c.num_boids];
	float next_dx[c.num_boids]; float next_dy[c.num_boids];

	randomize_boids(c.num_boids, c, curr_x, curr_y, curr_dx, curr_dy);

    // using pointers because buffer swapping
	float* curr_x_p  = (float*)curr_x;  float* curr_y_p  = (float*)curr_y;
	float* curr_dx_p = (float*)curr_dx; float* curr_dy_p = (float*)curr_dy;
	float* next_x_p  = (float*)next_x;  float* next_y_p  = (float*)next_y;
	float* next_dx_p = (float*)next_dx; float* next_dy_p = (float*)next_dy;
 
 	for(int i = 0; i<c.num_warmup_iterations; ++i) {
        soa_iteration(c.num_boids, c,
                      &curr_x_p, &curr_y_p, &curr_dx_p, &curr_dy_p,
                      &next_x_p, &next_y_p, &next_dx_p, &next_dy_p);
	}
    
    long long millis = current_millis();
 	for(int i = 0; i<c.num_iterations; ++i) {
        soa_iteration(c.num_boids, c,
                      &curr_x_p, &curr_y_p, &curr_dx_p, &curr_dy_p,
                      &next_x_p, &next_y_p, &next_dx_p, &next_dy_p);
	}
    millis = current_millis() - millis;
    log_to_file(c.output_file, millis, c);

    return 0;
}
