#ifndef BOIDS_ARGPARSE_H
#define BOIDS_ARGPARSE_H

/**
 * functions for
 * creating command line flag schema
 * parsing (and validating) argv according to schema in question 
 * set program variables to values specified in argv
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef enum {ARGPARSE_INT, ARGPARSE_FLOAT, ARGPARSE_STRING} argparse_type;

// can't be fucked to make a has map in c
// we're doing this alist style the way god intended
struct arg_specs {
	const char* flag_name;
	void* ptr;
	const char* help;
	argparse_type type;
	struct arg_specs* next;
};

typedef struct arg_specs arg_specs;

extern arg_specs* global_arg_specs;

void argparse_bind(const char* argname, void* ptr, const char* help,
				   argparse_type type);
void argparse_bind_int(const char* argname, int* ptr, const char* help);
void argparse_bind_string(const char* argname, char** ptr, const char* help);
void argparse_bind_float(const char* argname, float* ptr, const char* help);

const char* argparse_stringify_type(argparse_type type);
const char* argparse_strip_leading_dashes(const char* c);

arg_specs* argparse_find_spec(const char* name);
int argparse_assign_values(int argc, char** argv);

void argparse_print_usage(const char* prog_name);
void argparse_print_flag_help(const arg_specs* as);
void argparse_print_flag_help_name(const char* name);
void argparse_print_flags_help();
void argparse_print_help(char* prog_name);

void argparse_free();

#endif
