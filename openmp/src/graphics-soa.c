#include<omp.h>            
#include"args.h"            // some global variables and argv parsing
#include"graphics-common.h" // raylib and some boid draw logic

#include"soa.h"
 
void boids_draw_all(size_t n_boids,
					const float x[n_boids], const float y[n_boids],
					const float dx[n_boids], const float dy[n_boids]) {
	for(size_t i = 0; i<n_boids; ++i) {
		draw_boid_coords(x[i], y[i], dx[i], dy[i]);
	}
}
 
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
 
 	InitWindow(c.window_width, c.window_height, "nomen fenetrae");
 	SetTargetFPS(c.window_fps);

    /*
     * swapping arrays in c doesn't work too well, to be able to swap them
     * we shall therefore have to handle them as pointers
     */
	float* curr_x_p  = (float*)curr_x;  float* curr_y_p  = (float*)curr_y;
	float* curr_dx_p = (float*)curr_dx; float* curr_dy_p = (float*)curr_dy;
	float* next_x_p  = (float*)next_x;  float* next_y_p  = (float*)next_y;
	float* next_dx_p = (float*)next_dx; float* next_dy_p = (float*)next_dy;
 
 	while(!WindowShouldClose()) {
 		BeginDrawing();
		ClearBackground(BLACK);

        boids_draw_all(c.num_boids, curr_x, curr_y, curr_dx, curr_dy);
        soa_iteration(c.num_boids, c,
                      &curr_x_p, &curr_y_p, &curr_dx_p, &curr_dy_p,
                      &next_x_p, &next_y_p, &next_dx_p, &next_dy_p);

		c.window_width = GetScreenWidth();
		c.window_height = GetScreenHeight();
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
