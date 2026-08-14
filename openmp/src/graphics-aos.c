#include<omp.h>
#include"args.h"            // some global config variables, and parse_args
#include"graphics-common.h" // raylib, and some drawing logic

#include"aos.h"

void draw_boid_struct(const boid b) {
	draw_boid_coords(b.curr_x, b.curr_y, b.curr_dx, b.curr_dy);
}
 
void boids_draw_all(size_t nboids, boid boids[nboids]) {
	for(size_t i = 0; i<nboids; ++i) {
		draw_boid_struct(boids[i]);
	}
}
 
int main(int argc, char** argv) {
	config c;
	config_set_default(&c);
	handle_argv_vars(argc, argv, &c);
    omp_set_num_threads(c.num_threads);

	boid boids[c.num_boids];
	randomize_boids(c.num_boids, c, boids);
 
 	InitWindow(c.window_width, c.window_height, "nomen fenetrae");
 	SetTargetFPS(c.window_fps);
 
 	while(!WindowShouldClose()) {
 		BeginDrawing();
		ClearBackground(BLACK);

		boids_draw_all(c.num_boids, boids);
		boids_update_all(c.num_boids, c, boids);

		// make boid simulation adapt to window resizing
		// updating here avoids weird race conditions
		c.window_width = GetScreenWidth();
		c.window_height = GetScreenHeight();
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
