#include"argparse.h"

arg_specs* global_arg_specs = NULL;

void argparse_bind(const char* argname, void* ptr, const char* help,
				   argparse_type type) {
	arg_specs* next = (arg_specs*)malloc(sizeof(arg_specs));
	*next = (arg_specs){
		.flag_name = argname,
		.ptr = ptr,
		.help = help,
		.type = type,
		.next = NULL
	};
	if(global_arg_specs == NULL) {
		global_arg_specs = next;
		return;
	}

	arg_specs* prev = global_arg_specs;
	arg_specs* curr = prev->next;
	while(curr != NULL) {
		prev = prev->next;
		curr = curr->next;
	}
	prev->next=next;
}

void argparse_bind_int(const char* argname, int* ptr, const char* help) {
	argparse_bind(argname, (void*)ptr, help, ARGPARSE_INT);
}

void argparse_bind_string(const char* argname, char** ptr, const char* help) {
	argparse_bind(argname, (void*)ptr, help, ARGPARSE_STRING);
}

void argparse_bind_float(const char* argname, float* ptr, const char* help) {
	argparse_bind(argname, (void*)ptr, help, ARGPARSE_FLOAT);
}


const char* argparse_stringify_type(argparse_type type) {
	switch(type) {
	case ARGPARSE_INT: return "integer";
	case ARGPARSE_STRING: return "string";
	case ARGPARSE_FLOAT: return "float";
	default:
		fprintf(stderr, "argparse_stringify_type: unrecognized type");
		return "if you're reading this the author fucked something up";
	}
}

const char* argparse_strip_leading_dashes(const char* c) {
	const char* stripped = c;
	while(*stripped=='-')stripped++;
	return stripped;
}


arg_specs* argparse_find_spec(const char* name) {
	for(arg_specs* as = global_arg_specs; as; as = as->next) {
		if(strcmp(name, as->flag_name) == 0)
			return as;
	}
	fprintf(stderr, "argparse_get_spec: found no spec for name %s\n", name);
	return NULL;
}

// assume values are structured like
// <flag-name> <flag-vals> <flag-name> <flag-vals> ...
int argparse_assign_values(int argc, char** argv) {
	// skip argv[0]
	argc--;
	argv++;
	
	if(argc%2) {
		fprintf(stderr, "oops! odd number of arguments expected even number"
				", all arguments must be single and preceded by their key\n");
		return 0;
	}

	for(int i = 0; i<argc; i+=2) {
		arg_specs* spec = argparse_find_spec(argv[i]);
		if(!spec) {
			fprintf(stderr, "oops! found '%s' argument but I don't know"
					" how to read it\n", argv[i]);
			return 0;
		}
			
		switch(spec->type) {
		case ARGPARSE_INT:
			*((int*)spec->ptr) = atoi(argv[i+1]);
			break;
		case ARGPARSE_FLOAT:
			// idk if there's a stoi equivalent for floats stof
			sscanf(argv[i+1], "%f", (float*)spec->ptr);
			break;
		case ARGPARSE_STRING:
			*((char**)spec->ptr) = argv[i+1];
			break;
		}
	}
	return 1;
}


void argparse_print_usage(const char* prog_name) {
	printf("usage: %s ", prog_name);
	for(arg_specs* as = global_arg_specs; as; as = as->next) {
		printf("[%s %s]", as->flag_name,
			   argparse_strip_leading_dashes(as->flag_name));
	}
	puts("");
}

void argparse_print_flag_help(const arg_specs* as) {
	printf("%s: default value[", as->flag_name);
	switch(as->type) {
	case ARGPARSE_INT:
		printf("%d", *((int*)as->ptr));
		break;
	case ARGPARSE_FLOAT:
		printf("%f", *((float*)as->ptr));
		break;
	case ARGPARSE_STRING:
		printf("\"%s\"", *((char**)as->ptr));
		break;
	default:
		fprintf(stderr, "argparse_print: unrecognized type");
	}
	printf("] type:[%s]", argparse_stringify_type(as->type));
	
	if(strcmp(as->help, ""))
		printf("\n%s\n", as->help);
	else
		puts("");
}

void argparse_print_flag_help_name(const char* name) {
	argparse_print_flag_help(argparse_find_spec(name));
}

void argparse_print_flags_help() {
	for(arg_specs* as = global_arg_specs; as; as = as->next) {
		argparse_print_flag_help(as);
	}
}

void argparse_print_help(char* prog_name) {
	argparse_print_usage(prog_name);
	argparse_print_flags_help();
}


void argparse_free() {
	for(arg_specs* as = global_arg_specs; as;) {
		arg_specs* old = as;
		as = as->next;
		free(old);
	}
}
