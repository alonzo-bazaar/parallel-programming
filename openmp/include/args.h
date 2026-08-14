#ifndef BOIDS_ARGS_H
#define BOIDS_ARGS_H

#include "argparse.h"
#include "config.h"

void bind_config_vars(config* c);
void print_help();
void handle_argv_vars(int argc, char** argv, config* c);

#endif
