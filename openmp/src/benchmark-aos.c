#include<omp.h>
#include"args.h"
#include"benchmark-common.h"

#include"aos.h"

int main(int argc, char** argv) {
	config c;
	config_set_default(&c);
	handle_argv_vars(argc, argv, &c);
    omp_set_num_threads(c.num_threads);

	boid boids[c.num_boids];
	randomize_boids(c.num_boids, c, boids);
 
 	for(int i = 0; i<c.num_warmup_iterations; ++i)
		boids_update_all(c.num_boids, c, boids);
    
    long long millis = current_millis();
 	for(int i = 0; i<c.num_iterations; ++i)
		boids_update_all(c.num_boids, c, boids);
    millis = current_millis() - millis;

    log_to_file(c.output_file, millis, c);

    return 0;
}
