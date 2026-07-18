//// FILE HEADER
//// ----------------------------------------------------------------------------
// SHSL: Single Header Scripting (Library|Language|Layer|Lisp)
// embeddable interpreter for a small lisp language
// localized entirely within a C header file
// shsl is distributed under the GNU LGPL v2.1 (see LICENSE file for details)
// author: Alonzo Bazaar <alonzo.lo.stronzo@protonmail.com>
// (that is indeed a pseudonym)

#ifndef SHSL_H
#define SHSL_H

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdarg.h>

#include<ctype.h>
#include<string.h>
#include<assert.h>
#include<errno.h>

// for fork/exec (to be moved to shsl_exec.h when the time comes)
#include<unistd.h>
#include<sys/wait.h>

//// DUMB UTILITIES DECLARATIONS
//// ----------------------------------------------------------------------------
// 
// I'm putting all the utils&Co. upfront instead of putting them closer to where
// they're used as to avoid interspersing utility code with more business
// logicky code, there still are utilities in the business logicky parts of this
// codebase, of course, but they're utilities pertaining to the business logic
// 
// these first utilities are instead completely unrelated to the fact they're
// part of shsl and are just a bunch of functions that were nice to have for this
// so I added em to here

/// NAMESPACING UTILITIES DECLARATIONS (preprocessor abuse)
#define defstruct(s) struct s; typedef struct s s

/// GENERIC PREPROCESSOR ABUSE
// adapted from
// https://groups.google.com/g/comp.std.c/c/d-6Mj5Lko_s?pli=1
// found in the second anser to
// https://stackoverflow.com/questions/2124339/
// C++ preprocessor __VA_ARGS__ number of arguments
// 
// from 1 to 29 arguments, this macro will return the number of arguments
// passed to the macro
#define SHSL_ARG_COUNT(...) SHSL_EXTRACT_30TH_IN        \
    (__VA_ARGS__,                                       \
     29,28,27,26,25,24,23,22,21,20,                     \
     19,18,17,16,15,14,13,12,11,10,                     \
     9,8,7,6,5,4,3,2,1)
// if __VA_ARGS__ has 1 element __VA_ARGS__, 29, 28, ... will have 30 args
// and the 30th will be 1
//
// with 2 argument the sequence will have 31 args
// and the 30th will be 2
//
// with 3 argument the sequence will have 32 args
// and the 30th will be 3
// 
// so the 30th argument of __VA_ARGS__, 29, 28... is always the amount of
// arguments that were present in __VA_ARGS__
// 
// to extract the 30th element in an argument list we quite "simply"
#define SHSL_EXTRACT_30TH_IN(_01, _02, _03, _04, _05,   \
                             _06, _07, _08, _09, _10,   \
                             _11, _12, _13, _14, _15,   \
                             _16, _17, _18, _19, _20,   \
                             _21, _22, _23, _24, _25,   \
                             _26, _27, _28, _29, _30,   \
                             ...) _30

/// CHARACTER AND STRING HANDLING DECLARATIONS
defstruct(shsl_string_builder);
void shsl_sb_push(shsl_string_builder* sb, const char c);
void shsl_sb_push_nullt_str(shsl_string_builder* sb, const char* c);
void shsl_sb_push_sized_str(shsl_string_builder* sb, const char* c, size_t size);
char* shsl_sb_get(shsl_string_builder* sb);

char* shsl_cat_strs_n(size_t n, ...);
char* shsl_append_chars_n(const char* s, size_t n, ...);
// concatenate a bunch of strings
#define shsl_cat_strs(...)                                      \
    shsl_cat_strs_n(SHSL_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
// append a bunch characters at the end of a string
#define shsl_append_chars(first, ...)                                   \
    shsl_append_chars_n(first, SHSL_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
bool is_escape_char(const char c);
bool does_char_escape(const char c);
char escape_to_escaped(const char c);

// more or less
// https://doc.rust-lang.org/std/primitive.never.html 
// born to typecheck, live to crash and die
#define shsl_dec_errtype_fn(type, name)                 \
    type shsl_err_ ## name ## _fn                       \
    (const char* errmsg, size_t line, const char* file)

#define shsl_def_errtype_fn(type, name)                         \
    type shsl_err_ ## name ## _fn                               \
    (const char* errmsg, size_t line, const char* file) {       \
        fprintf(stderr, "some shit failed!\n%s\n", errmsg);     \
        fprintf(stderr, "at line %zu in file %s", line, file);  \
        exit(1);                                                \
    }

shsl_dec_errtype_fn(int, int);
shsl_dec_errtype_fn(size_t, size_t);
shsl_dec_errtype_fn(ssize_t, ssize_t);
shsl_dec_errtype_fn(const char*, const_str);
shsl_dec_errtype_fn(char*, str);

#define shsl_err_int(msg) shsl_err_int_fn(msg, __LINE__, __FILE__)
#define shsl_err_size_t(msg) shsl_err_size_t_fn(msg, __LINE__, __FILE__)
#define shsl_err_const_str(msg) shsl_err_const_str_fn(msg, __LINE__, __FILE__)
#define shsl_err_str(msg) shsl_err_str_fn(msg, __LINE__, __FILE__)


//// SHSL DATA DECLARATIONS
//// ----------------------------------------------------------------------------
/// DATA TYPES DECLARATIONS
// type tag for shsl objects
typedef enum SHSL_OBJ_TYPE {
    // atoms
    // nil first so an object initialized as {0} is nil
    SHSL_NIL = 0, SHSL_SYM, 
    SHSL_INT, SHSL_REAL, SHSL_STR,

    // collections
    SHSL_CONS, SHSL_VEC, SHSL_MAP,

    // functions and macros
    SHSL_BUILTIN_FN, SHSL_USER_FN, 
    SHSL_BUILTIN_MACRO, SHSL_USER_MACRO, 

    // error
    SHSL_ERR,
} SHSL_OBJ_TYPE;

// tagged union of all possible shsl objects
defstruct(shsl_obj);
// possible shsl objects
// (outside of primitive types for which we just use the C ones)
defstruct(shsl_sym);
defstruct(shsl_err);
defstruct(shsl_cons);
defstruct(shsl_vec);
// key value pair (for maps)
// kv pairs are not shsl objects, they're defined separately just for ease 
// other than that they're just there to aid in defining maps
defstruct(shsl_kv);
// map as vector of kv pairs
defstruct(shsl_map);
defstruct(shsl_builtin_fn);
// lambda list is a separate struct because lambda lists get complex rather fast
defstruct(shsl_lambda_list);
defstruct(shsl_user_fn);
// we don't have macro types as macros are just gonna be functions with different
// type tags that we run at a different moment
// (compile/expand time instead of runtime)

// we use references instead of pointers
// references hold a pointer, and some extra information, such as
// is this a weak reference?
// (in the future) is this a mutable reference?
// and so on.
// references are ususally allocated on the stack
defstruct(shsl_ref);

extern shsl_obj SHSL_GLOBAL_NIL;
shsl_ref shsl_ref_to_nil(void);

/// REFERENCE OPERATIONS DECLARATIONS
// creates initial reference struct from object pointer
shsl_ref shsl_ref_from_ptr(shsl_obj* ptr);
// creates initial reference struct from object pointer
// but it's a weak reference
shsl_ref shsl_weak_ref_from_ptr(shsl_obj* ptr);
// marks addition of reference to object
// also returns ref to modified object for convenience
shsl_ref shsl_ref_add(shsl_ref obj);
// marks removal of reference to object
// may also delete the object if reference count reaches 0 
void shsl_ref_drop(shsl_ref obj);
// marks a reference as weak, if reference was previously not weak
// also decreases reference count of object it points to to make sure
// it doesn't keep anything alive by itself
void shsl_ref_mark_weak(shsl_ref* ref);
// dst = src, so src overwrites dst
// previous object pointed by dst loses a ref
// and object pointed by src gains a ref
// only use when overwriting an old value, do not use for initializatin code 
void shsl_ref_set(shsl_ref* dst, shsl_ref src);
// the c code deals with a lot of references that may have ref count of 0
// (the refcount is an shsl thing, the c code kinda does whatever)
// sometimes that's desired behaviour
// when working with fresh objects that haven't benn bound yet
// or when dealing with transient objects
// then the time comes when you're holding an object and are like 
// idk if this has been bound to anything while I was using it
// if it hasn't been bound to anything just burn it
// TODO: better name(?)
void shsl_ref_free_if_unreachable(shsl_ref ref);

/// DATA PREDICATES DECLARATIONS
SHSL_OBJ_TYPE shsl_type(const shsl_ref ref);
bool shsl_is_nil(const shsl_ref ref);
bool shsl_is_sym(const shsl_ref ref);
bool shsl_is_str(const shsl_ref ref);
bool shsl_is_int(const shsl_ref ref);
bool shsl_is_real(const shsl_ref ref);
bool shsl_is_num(const shsl_ref ref);
bool shsl_is_err(const shsl_ref ref);
bool shsl_is_truthy(const shsl_ref ref);

bool shsl_is_cons(const shsl_ref ref);
bool shsl_is_list(const shsl_ref ref);
bool shsl_is_proper_list(const shsl_ref list_ref);
bool shsl_is_vec(const shsl_ref ref);
bool shsl_is_map(const shsl_ref ref);

bool shsl_is_builtin_fn(const shsl_ref ref);
bool shsl_is_user_fn(const shsl_ref ref);
bool shsl_is_builtin_macro(const shsl_ref ref);
bool shsl_is_user_macro(const shsl_ref ref);
bool shsl_is_fn(const shsl_ref ref);
bool shsl_is_macro(const shsl_ref ref);

/// DATA CONSTRUCTION DECLARATIONS
// we handle everything through references/pointers because havnig everything
// on the heap makes it easier to automatically manage
shsl_ref shsl_mkint(long l);
shsl_ref shsl_mkreal(double d);
shsl_ref shsl_mkstr(const char* str);
shsl_ref shsl_mksym(const char* name);
shsl_ref shsl_mkerr(shsl_ref data, const char* msg, ...);
// shsl_vmkerr is like shsl_mkerr but accepts a "pre opened" va_list and it
// doesn't call va_close on the list, it so you can call it with an outside
// va_list if you wanna wrap error generation around another variadic function
shsl_ref shsl_vmkerr(shsl_ref data, const char* msg, va_list args);
shsl_ref shsl_mkcons(shsl_ref car, shsl_ref cdr);
shsl_ref shsl_mkmap(size_t initial_capacity);
shsl_ref shsl_mkvec(size_t initial_capacity);
shsl_ref shsl_mkbuiltin_fn(shsl_ref env,
                           shsl_ref(*apply)(shsl_ref args,
                                            shsl_ref env));
shsl_ref shsl_mkbuiltin_macro(shsl_ref env,
                              shsl_ref(*apply)(shsl_ref args,
                                               shsl_ref env));
// this one bit is kinda ugly
struct shsl_expr;
shsl_ref shsl_mkuser_fn(shsl_ref env, shsl_lambda_list* lambda_list,
                        struct shsl_expr** body, size_t body_length);
shsl_ref shsl_mkuser_macro(shsl_ref env, shsl_lambda_list* lambda_list,
                           struct shsl_expr** body, size_t body_length);

/// DATA OPERATIONS DECLARATIONS
/// GENERIC OPERATIONS DEFINITIONS
// copy, free, equality
// creates fresh (newly allocated) deep copy of obj
// doesn't copy refcount tho
// fresh objects are initialized with a starting refcount of 0
shsl_ref shsl_copy(shsl_ref obj);
// deletes the object and removes reference to any eventual sub-objects
// (cons car and cdr, vector elements, map keys and values, et al.)
void shsl_free(shsl_ref obj);
// we only have one equality test, and not a couple dozen, like other lisps
// all equalities here are basically #'equal, deep compare, fuck it
// we don't care about sameness or equivalence or whatnot, just #'equal
bool shsl_eq(shsl_ref lhs, shsl_ref rhs);

/// CONS MANIPULATIONS DECLARATIONS
void shsl_set_car(shsl_ref cons_obj, shsl_ref car);
void shsl_set_cdr(shsl_ref cons_obj, shsl_ref cdr);

/// VECTOR MANIPULATIONS DECLARATIONS
void shsl_vec_expand(shsl_ref vec_obj, size_t new_size);
void shsl_vec_push(shsl_ref vec_obj, shsl_ref obj);
shsl_ref shsl_vec_get(shsl_ref vec_obj, size_t i);
void shsl_vec_set(shsl_ref vec_obj, size_t i, shsl_ref new_val);
size_t shsl_vec_length(shsl_ref vec_obj);
shsl_ref shsl_vec_first(shsl_ref vec_obj);
shsl_ref shsl_vec_last(shsl_ref vec_obj);
shsl_ref shsl_vec_shallow_copy(shsl_ref vec_obj);
shsl_ref shsl_vec_slice(shsl_ref vec_obj, size_t first, size_t last);
// bit of shit, but this avoids polluting the body with extra symbols
// although it does introduce a bit of overhead, sorry :|
// (it should be easy enough for the compiler to remove the extra overhead)
// it's here and not in the implementation as this is part of shsl's api
// not the way to place to put an assert in but hey, if it works it works
#define shsl_vec_foreach(i, elt, vec_ref)                               \
    for(size_t i = shsl_is_vec(vec_ref)?0:shsl_err_size_t               \
            ("can't call shsl_vec_foreach on non vector datum!");       \
        i<vec_ref.ptr->vec.size; ++i)                                   \
	for(shsl_ref elt = shsl_vec_get(vec_ref, i);                    \
	    elt.ptr; elt.ptr = NULL)                                    \

/// MAP MANIPULATIONS DECLARATIONS
void shsl_map_expand(shsl_ref map_obj, size_t new_size);
// it's ssize_t instead of size_t so we can use -1 to say "hey, didn't find shit"
ssize_t shsl_map_index(shsl_ref map_obj, shsl_ref key);
shsl_ref shsl_map_get(shsl_ref map_obj, shsl_ref key);
void shsl_map_set(shsl_ref map_obj,
		  shsl_ref key, shsl_ref new_val);

/// COLLECTION BUILDERS DECLARATIONS
typedef enum SHSL_CB_TYPE {SHSL_CB_LIST, SHSL_CB_VEC, SHSL_CB_MAP} SHSL_CB_TYPE;
defstruct(shsl_cons_builder);
defstruct(shsl_vec_builder);
defstruct(shsl_map_builder);
defstruct(shsl_cb);
shsl_cb shsl_cb_make(SHSL_CB_TYPE type);
void shsl_cb_add(shsl_cb* cb, shsl_ref obj);
shsl_ref shsl_cb_get(shsl_cb);

/// LIST OPERATIONS DECLARATIONS
shsl_ref shsl_car(shsl_ref ref);
shsl_ref shsl_cdr(shsl_ref ref);
shsl_ref shsl_nthcdr(shsl_ref ref, size_t n);
shsl_ref shsl_nth(shsl_ref list_ref, size_t n);
// ssize_t so we can use -1 as "bro what the fuck is this object"
ssize_t shsl_list_length(shsl_ref list_ref);
char* shsl_sym_name(shsl_ref sym_ref);

long shsl_int(shsl_ref ref);
double shsl_real(shsl_ref ref);

/// FUNCTION OPERATIONS DECLARATIONS
// we have 4 different function types so it's easier to have separate functions
// for these tasks
shsl_ref shsl_fn_env(shsl_ref ref);
void shsl_fn_env_mark_weak(shsl_ref ref);


//// LEXER DECLARATIONS
//// ----------------------------------------------------------------------------

// possible types a token may assume
typedef enum SHSL_TOKEN_TYPE {
    // literals
    SHSL_TOK_NIL = 0, SHSL_TOK_SYMBOL,
    SHSL_TOK_INTEGER, SHSL_TOK_REAL, SHSL_TOK_STRING,

    // (quasi)quoting
    SHSL_TOK_QUOTE, SHSL_TOK_QUASIQUOTE, SHSL_TOK_COMMA,

    // parentheses
    SHSL_TOK_OPEN_PAREN, SHSL_TOK_CLOSE_PAREN,
    SHSL_TOK_OPEN_SQUARE, SHSL_TOK_CLOSE_SQUARE,
    SHSL_TOK_OPEN_CURLY, SHSL_TOK_CLOSE_CURLY,

    // we then have special token types to express
    // eof
    SHSL_TOK_EOF,
    // error
    SHSL_TOK_ERROR,
} SHSL_TOKEN_TYPE;
// TODO: not a tagged union yet but I'm working on it
defstruct(shsl_token);
// pair of read token/remaining string after token
defstruct(lexer_pair);

// return a token of a given type with no extra information
shsl_token empty_token(enum SHSL_TOKEN_TYPE token_type);
// return an error message from a token_off call
lexer_pair error_lexer_pair(const char* errmsg);

// is this character special for the lexer?
bool is_special_char(char c);
// can this a character that be be part of a symbol (or number)
bool is_symbol_char(char c);
// try parsing from c to c+len as an integer and writing that into *into
// returns true if parsing was succesful, false otherwise
bool try_parse_integer(char* c, size_t len, long* into);
// copy characters from c to c+len into a fresh null terminated string
char* slice_to_fresh_str(const char* c, size_t len);
// parse a non special token (symbol or number) starting at c
shsl_token parse_non_special_token(char*c, size_t len);

// read a token off of string and return the pair of that token plus
// the string after the token
lexer_pair token_off(char* str);


//// PARSER DECLARATIONS
//// ----------------------------------------------------------------------------

// pair of parsed object/remaining string after object
// same logic as lexer_pair
defstruct(parser_pair);
// parse object starting at str and return pair of that object
// plus the string after that object
parser_pair parse_off(char* str);
// parse from str until you meet a token of stop type
// collect the results into a list and return them
// erros out if it meets a token of type error_on[0]...error_on[error_on_len] before
// a token of type stop
// 
// said less abstractly
// if you meet a '(' you parse until ')' and error out if you meet a ']' or '}'
// at the same depth before you meet a ')' at the same depth
parser_pair parse_until(char* str,
			shsl_cb cb,
			enum SHSL_TOKEN_TYPE stop,
			enum SHSL_TOKEN_TYPE* error_on, size_t error_on_length);


//// EVALUATOR DECLARATIONS
//// ----------------------------------------------------------------------------

/// EXPRESSION TYPE DECLARATIONS
typedef enum SHSL_EXPR_TYPE {
    SHSL_EXPR_LITERAL,
    SHSL_EXPR_VEC,
    SHSL_EXPR_MAP,
    SHSL_EXPR_LOOKUP,
	
    SHSL_EXPR_IF,
    SHSL_EXPR_LET,
    SHSL_EXPR_WHILE,
    SHSL_EXPR_DO,
    SHSL_EXPR_DO_POKING,

    SHSL_EXPR_DEF,
    SHSL_EXPR_SET,

    SHSL_EXPR_FN,
    SHSL_EXPR_MACRO,

    SHSL_EXPR_FUNCALL,
} SHSL_EXPR_TYPE;
defstruct(shsl_vec_expr);
defstruct(shsl_map_expr);
defstruct(shsl_if_expr);
defstruct(shsl_do_expr);
defstruct(shsl_do_poking_expr);
defstruct(shsl_def_expr);
defstruct(shsl_set_expr);
defstruct(shsl_fn_expr);
defstruct(shsl_macro_expr);
defstruct(shsl_funcall_expr);
// tagged union of all possible expression types
// (either the ones above or like, literals and shit)
defstruct(shsl_expr);

/// EXPRESSION FUNCTIONS DECLARATIONS
// is parsed expression a parser error?
bool shsl_expr_is_error(shsl_expr* expr);
// return parser error
shsl_expr* shsl_expr_error(shsl_ref form, const char* msg, ...);
// return parser but accepts pre "opened" va_list and doesn't call va_close
// on args, this way parser error generation can be wrapped in an outside
// variadic function
shsl_expr* shsl_vexpr_error(shsl_ref form, const char* msg, va_list args);
// returns a parser error caused by another parser error
shsl_expr* shsl_expr_further_error(shsl_expr* orig, const char* msg, ...);

/// PARSING UTILITIES DECLARATIONS
// parse a (cons) list of forms into an array of expression pointers
// if it encounters a parser error it adds the error to the array then stops
shsl_expr** shsl_form_list_to_expr_arr(shsl_ref form, shsl_ref env);
// return index of first parser error in expression array
// or -1 if none of the expressions in the array are parser errors
ssize_t shsl_expr_arr_find_err(shsl_expr** arr, size_t len);
// frees the first len expressions in expression array, then frees the array
// len could be the length of the allocated array or just a prefix of it
// 'cause if we encounter an error we don't construct any expressions
// past the error, so we don't wanna free un malloc'd pointers
void shsl_free_expr_arr(shsl_expr** arr, size_t len);

/// FORM TRANSLATION FUNCTIONS DECLARATIONS
// translate shsl object form representing some expression into corresponding
// expression object
// takes an env as it also handles macro expansion and thus needs a place to look
// those macros up in case
shsl_expr* shsl_form_to_expr(shsl_ref form, shsl_ref env);

// free expression object (expression objects are malloc'd)
void shsl_expr_free(shsl_expr* expr);

/// EVALUTATION FUNCTIONS DECLARATIONS

// shsl uses the environment mode of evaluation from sicp
// we don't have a separate shsl_environment type as the enviroment will be
// represented using a cons list of shsl maps
// this should make for easier debuggability, even from within shsl
// and may allow for some funky pythonish introspection

// create enviroment frame (map) binding given syms to given vals
shsl_ref shsl_env_mkframe(shsl_ref syms, shsl_ref vals);
// create enviroment frame (map) binding given lambda list and values
shsl_ref shsl_ll_env_mkframe(shsl_lambda_list* syms, shsl_ref vals);
// find kv pair in environment correspinding to key, or NULL if no such pair exists
shsl_kv* shsl_env_find_kv(shsl_ref env, shsl_ref key);
shsl_ref shsl_env_lookup(shsl_ref env, shsl_ref key);
shsl_ref shsl_env_lookup_or(shsl_ref env, shsl_ref key, shsl_ref def);
bool shsl_env_has(shsl_ref env, shsl_ref key);
shsl_ref shsl_env_set(shsl_ref env, shsl_ref key, shsl_ref new_val);
shsl_ref shsl_env_def(shsl_ref env, shsl_ref key, shsl_ref new_val);

// calling functions (for eval, and also other shit)
shsl_ref shsl_call_builtin_fn(shsl_ref fn, shsl_ref args);
shsl_ref shsl_call_user_fn(shsl_ref fn, shsl_ref args);

// evaluate expression and return its value
shsl_ref shsl_eval(shsl_expr* form, shsl_ref env);
shsl_ref shsl_eval_many_into_vec(shsl_expr** args, size_t args_length,
                                 shsl_ref env);
shsl_ref shsl_eval_sequence(shsl_expr** seq, size_t seq_length, shsl_ref env);

/// BUILTIN FUNCTIONS DECLARATIONS
// that is, definition of the functions that will be visible as builtins
// in the shsl code

/// SHSL BUILTIN ARITHMETIC FUNCTIONS DECLARATIONS
shsl_ref shsl_builtin_add(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_sub(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_mul(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_div(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_gt(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_lt(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_ge(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_le(shsl_ref args, shsl_ref env);

/// SHSL BUILTIN DATA FUNCTIONS DECLARATIONS
shsl_ref shsl_builtin_isnil(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_isint(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_isreal(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_isnum(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_issym(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_isstr(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_iserr(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_iscons(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_isvec(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_ismap(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_islist(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_isfn(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_ismacro(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_err(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_cons(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_car(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_cdr(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_nth(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_nthcdr(shsl_ref args, shsl_ref env);

shsl_ref shsl_builtin_vecget(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_vecset(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_vecpush(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_veclen(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_veccat(shsl_ref args, shsl_ref env);

// TODO: for when I get destructuring set to work
// mapget should return a pair/length 2 vector/whatever [value  foundp]
shsl_ref shsl_builtin_mapget(shsl_ref args, shsl_ref env);
// [value was-alread-present-p] ?
shsl_ref shsl_builtin_mapset(shsl_ref args, shsl_ref env);

/// SHSL BUILTIN OTHER FUNCTIONS DECLARATIONS
shsl_ref shsl_builtin_equal(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_pr(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_prn(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_print(shsl_ref args, shsl_ref env);
shsl_ref shsl_builtin_println(shsl_ref args, shsl_ref env);

// and given all these builtin functions we can create the initial environment
// for shsl code evaluation
shsl_ref shsl_env_mkinitial(void);

//// PRINT DEBUGGING DECLARATIONS
//// ----------------------------------------------------------------------------
/// SERIALIZATION DECLARATIONS
void shsl_sb_push_obj(shsl_string_builder* sb, shsl_ref obj, bool pretty);
char* shsl_to_string(shsl_ref ref);

/// PRINTING DECLARATIONS
void shsl_dbg_fputtok(const shsl_token* tok, FILE* stream);
void shsl_fpr(const shsl_ref obj, FILE* stream);
void shsl_fprint(const shsl_ref obj, FILE* stream);

//// USER FACING FUNCTIONS DECLARATIONS
//// ----------------------------------------------------------------------------

/// SHSL SUBROUTINES FOR MAIN DECLARATIONS
// (they are, of course, also useable standalone and are often "pure" enough
//  that it makes sense to use em as such)
shsl_ref shsl_eval_str(char* c, shsl_ref env);
char* shsl_read_file(const char *path);
shsl_ref shsl_eval_file(const char* filepath, shsl_ref env);
void shsl_repl(shsl_ref env);

/// SHSL MAIN DECLARATIONS
void shsl_usage(const char* name, bool print_extra, FILE* stream);
void shsl_die(int exit_with, const char* errmsg, ...);
int shsl_main(int argc, char** argv);

#endif // SHSL_H

#ifdef SHSL_IMPLEMENTATION
//// DUMB UTILITIES DECLARATIONS
//// ----------------------------------------------------------------------------

/// CHARACTER AND STRING HANDLING DECLARATIONS
typedef struct shsl_string_builder {
    char* buf;
    size_t size;
    size_t capacity;
} shsl_string_builder;

void shsl_sb_push(shsl_string_builder* sb, const char c) {
    if(sb->size == sb->capacity) {
        size_t new_capacity = sb->capacity+(sb->capacity/2)+1;
        sb->buf = (char*)realloc(sb->buf, new_capacity);
        sb->capacity = new_capacity;
    }
    sb->buf[sb->size]=c;
    sb->size++;
}
void shsl_sb_push_nullt_str(shsl_string_builder* sb, const char* c) {
    while(*c) shsl_sb_push(sb, *(c++));
}
void shsl_sb_push_sized_str(shsl_string_builder* sb, const char* c, size_t size) {
    for(size_t i = 0; i<size; ++i) shsl_sb_push(sb, c[i]);
}

// https://en.wikipedia.org/wiki/Escape_sequences_in_C

// does this character have to be escaped in strings?
bool is_escape_char(const char c) {
    return (c == '\a')
        || (c == '\b')
        // || (c == '\e') // (not ansi, -Wpedantic doesn't like it)
        || (c == '\f')
        || (c == '\n')
        || (c == '\r')
        || (c == '\t')
        || (c == '\v')
        || (c == '\\')
        || (c == '\"')
        || (c == '\?')
        || (c == '\0');
    // TODO: I'll get to unicode when I get to it
}
// given a character c, is \c a recognized escape sequence?
bool does_char_escape(const char c) {
    return (c == 'a')
        || (c == 'b')
        || (c == 'f')
        || (c == 'n')
        || (c == 'r')
        || (c == 't')
        || (c == 'v')
        || (c == '\\')
        || (c == '"')
        || (c == '?')
        || (c == '0');
}
// take character acting as "payload" in escape sequence
// (ie: the 'n' in '\n')
// and return the character it's escaping
char escape_to_escaped(const char c) {
    switch(c) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '\\': return '\\';
    case '"': return '\"';
    case '?': return '\?';
    case '0': return '\0';
    default: assert(0 && "unreachable");
    }
}
// take character and return representation of that
// character as when dumping to string
const char* to_string_representation(const char c) {
    static char b[2];
    switch(c) {
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    case '\v': return "\\v";
    case '\\': return "\\\\";
    case '\"': return "\\\"";
    case '\?': return "\\?";
    case '\0': return "\\0";
    default:
        b[0]=c;
        b[1]='\0';
        return b;
    }
}

char* shsl_cat_strs_n(size_t n, ...) {
    va_list args;
    va_start(args, n);
    shsl_string_builder sb = {0};
    for(size_t i = 0; i<n; ++i) {
        char* c = va_arg(args, char*);
        shsl_sb_push_nullt_str(&sb, c);
    }
    va_end(args);
    shsl_sb_push(&sb, '\0');
    return shsl_sb_get(&sb);
}

// https://stackoverflow.com/questions/23983471
// 'Char' is promoted to 'int' when passed through in C
char* shsl_append_chars_n(const char* s, size_t n, ...) {
    va_list args;
    va_start(args, n);
    shsl_string_builder sb = {0};
    shsl_sb_push_nullt_str(&sb, s);
    for(size_t i = 0; i<n; ++i) {
        char c = va_arg(args, int); // must be passed as int because C is stupid
        shsl_sb_push(&sb, c);
    }
    va_end(args);
    shsl_sb_push(&sb, '\0');
    return shsl_sb_get(&sb);
}

shsl_def_errtype_fn(int, int)
shsl_def_errtype_fn(size_t, size_t)
shsl_def_errtype_fn(ssize_t, ssize_t)
shsl_def_errtype_fn(const char*, const_str)
shsl_def_errtype_fn(char*, str)

//// SHSL DATA DEFINITIONS
//// ----------------------------------------------------------------------------

/// DATA TYPES DEFINITIONS
typedef struct shsl_ref {
    shsl_obj* ptr;
    bool is_weak;
} shsl_ref;

typedef struct shsl_sym {
    shsl_ref name; // must be string
} shsl_sym ;
typedef struct shsl_err {
    shsl_ref data; // can be any
    shsl_ref msg;  // must be string
} shsl_err;
typedef struct shsl_cons {
    shsl_ref car;
    shsl_ref cdr;
} shsl_cons ;
typedef struct shsl_vec {
    shsl_ref* buf;
    size_t size;
    size_t capacity;
} shsl_vec;
// map is currently an alist (because fuck me ig)
typedef struct shsl_kv {
    shsl_ref k;
    shsl_ref v;
} shsl_kv;
typedef struct shsl_map {
    shsl_kv* buf;
    size_t size;
    size_t capacity;
} shsl_map;
typedef struct shsl_builtin_fn {
    // env must be cons list of frames (maps)
    // args must be a vector
    shsl_ref env;
    shsl_ref(*apply)(shsl_ref args, shsl_ref env);
} shsl_builtin_fn;
typedef struct shsl_lambda_list {
    shsl_ref all;        // the entire parameter vector
    shsl_ref variadic;   // may be either symbol or nil
                         // if symbol then this function can bind variadic shit
                         // if nil then we can't bind variadic shit
    shsl_ref keyword;    // map of symbol - default value
} shsl_lambda_list;
typedef struct shsl_user_fn {
    // env must be cons list of maps
    // lambda_list be vector
    shsl_ref env;
    shsl_lambda_list* lambda_list;
    shsl_expr** body;
    size_t body_length;
} shsl_user_fn;

typedef struct shsl_obj {
    // header
    int ref_count;
    SHSL_OBJ_TYPE type;

    // data
    union {
	long i;
	double r;
	char* str;

	shsl_sym sym;
	shsl_cons cons;

	shsl_vec vec;
	shsl_map map;

	shsl_builtin_fn builtin_fn;
	shsl_user_fn user_fn;
	shsl_builtin_fn builtin_macro;
	shsl_user_fn user_macro;

	shsl_err err;
    };
} shsl_obj;	

shsl_obj SHSL_GLOBAL_NIL = {0};
shsl_ref shsl_ref_to_nil(void) {
    return (shsl_ref) {
        .ptr = &SHSL_GLOBAL_NIL,
        .is_weak = true,
    };
}

/// REFERENCE OPERATIONS DEFINITIONS
#define return_mallocd_obj(...) do {                    \
        shsl_ref ref = (shsl_ref){                      \
            .ptr = (shsl_obj*)malloc(sizeof(shsl_obj)), \
            .is_weak = false,                           \
        };                                              \
        *(ref.ptr) = (shsl_obj){__VA_ARGS__};           \
        return ref;                                     \
    } while(0)

shsl_ref shsl_ref_from_ptr(shsl_obj* ptr) {
    return (shsl_ref){
        .ptr = ptr,
        .is_weak = false,
    };
}
shsl_ref shsl_weak_ref_from_ptr(shsl_obj* ptr) {
    return (shsl_ref){
        .ptr = ptr,
        .is_weak = true,
    };
}
shsl_ref shsl_ref_add(shsl_ref ref) {
    if(ref.is_weak) {
#ifdef SHSL_LOG_ADD_REF
        printf("[SHSL GC] not adding ref to object %p\n", (void*)(ref.ptr));
        printf("[SHSL GC] "); shsl_fpr(ref, stdout); fputc('\n', stdout);
        printf("[SHSL GC] because ref is weak\n");
        printf("[SHSL GC] remains at refcount %d\n", ref.ptr->ref_count);
        fputc('\n', stdout);
#endif
        return ref;
    }

#ifdef SHSL_LOG_ADD_REF
    printf("[SHSL GC] adding ref to object %p\n", (void*)(ref.ptr));
    printf("[SHSL GC] "); shsl_fpr(ref, stdout); fputc('\n', stdout);
    printf("[SHSL GC] was at refcount %d\n", ref.ptr->ref_count);
    fputc('\n', stdout);
#endif

    if(shsl_type(ref) != SHSL_NIL)
	ref.ptr->ref_count++;
    return ref;
}
void shsl_ref_drop(shsl_ref ref) {
    if(ref.is_weak) {
#ifdef SHSL_LOG_DEL_REF
        printf("[SHSL GC] not deleting ref to object %p\n", (void*)(ref.ptr));
        printf("[SHSL GC] "); shsl_fpr(ref, stdout); fputc('\n', stdout);
        printf("[SHSL GC] because ref is weak\n");
        printf("[SHSL GC] remains at refcount %d\n", ref.ptr->ref_count);
        fputc('\n', stdout);
#endif
        return;
    }

#ifdef SHSL_LOG_DEL_REF
    fprintf(stdout, "[SHSL GC] deleting ref to object %p\n", (void*)(ref.ptr));
    fprintf(stdout, "[SHSL GC] "); shsl_fpr(ref, stdout); fputc('\n', stdout);
    fprintf(stdout, "[SHSL GC] was at refcount %d\n", ref.ptr->ref_count);
    fputc('\n', stdout);
#endif    
    if(shsl_type(ref) == SHSL_NIL)
	return;

    ref.ptr->ref_count--;
    if(ref.ptr->ref_count == 0)
	shsl_free(ref);
    else if(ref.ptr->ref_count < 0)
	// *else if*, not if
	// having just if here is reading (potentially) freed data
	// and that's no good
	fprintf(stderr, "now you fucked up!\n");
}
void shsl_ref_mark_weak(shsl_ref* ref) {
    if(!ref->is_weak) {
        shsl_ref_drop(*ref);
        ref->is_weak = true;
    }
}
void shsl_ref_set(shsl_ref* dst, shsl_ref src) {
    shsl_ref_add(src);
    shsl_ref_drop(*dst);
    dst->ptr = src.ptr;
    dst->is_weak = src.is_weak;
}
void shsl_ref_free_if_unreachable(shsl_ref ref) {
    if(ref.ptr->ref_count == 0)
	shsl_free(ref);
}

/// DATA PREDICATES DEFINITIONS
SHSL_OBJ_TYPE shsl_type(shsl_ref ref) {
    return ref.ptr->type;
}
bool shsl_is_nil(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_NIL;
}
bool shsl_is_sym(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_SYM;
}
bool shsl_is_str(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_STR;
}
bool shsl_is_int(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_INT;
}
bool shsl_is_real(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_REAL;
}
bool shsl_is_num(const shsl_ref ref) {
    return shsl_is_int(ref) || shsl_is_real(ref);
}
bool shsl_is_err(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_ERR;
}
bool shsl_is_falsey(const shsl_ref ref) {
    return (shsl_is_nil(ref) || shsl_is_err(ref));
}
bool shsl_is_truthy(const shsl_ref ref) {
    return !shsl_is_falsey(ref);
}

bool shsl_is_cons(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_CONS;
}
bool shsl_is_list(const shsl_ref ref) {
    return shsl_is_nil(ref) || shsl_is_cons(ref);
}
// TODO: infinite loops on circular lists
// I'm already refcounting so I have accepted those will be a pita
// maybe have a tortoise and hare function that detects circular lists
// bool is_circular_list(const shsl_obj* list_obj)
bool shsl_is_proper_list(const shsl_ref list_ref) {
    shsl_obj* list_obj = list_ref.ptr;
    while(true) {
	switch(list_obj->type) {
	case SHSL_NIL:
	    return true;
	case SHSL_CONS:
	    list_obj = list_obj->cons.cdr.ptr;
	    break;
	default:
	    return false;
	}
    }
}
bool shsl_is_vec(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_VEC;
}
bool shsl_is_map(const shsl_ref ref) {
    return shsl_type(ref) == SHSL_MAP;
}

bool shsl_is_builtin_fn(const shsl_ref ref) {
    return shsl_type(ref)==SHSL_BUILTIN_FN;
}
bool shsl_is_user_fn(const shsl_ref ref) {
    return shsl_type(ref)==SHSL_USER_FN;
}
bool shsl_is_builtin_macro(const shsl_ref ref) {
    return shsl_type(ref)==SHSL_BUILTIN_MACRO;
}
bool shsl_is_user_macro(const shsl_ref ref) {
    return shsl_type(ref)==SHSL_USER_MACRO;
}
bool shsl_is_fn(const shsl_ref ref) {
    // macros count as functions
    return shsl_is_builtin_fn(ref) || shsl_is_user_fn(ref)
	|| shsl_is_builtin_macro(ref) || shsl_is_user_macro(ref);
}
bool shsl_is_macro(const shsl_ref ref) {
    return shsl_is_builtin_macro(ref) || shsl_is_user_macro(ref);
}

/// DATA CONSTRUCTION DEFINITIONS
// the shsl_mk* functions all return references to fresh objects
// although those objects may contain references to pre-existing objects
// if they're cons cells, vectors, or maps, for instance
// 
// all raw C data passed to an shsl_mk* constructor
// (numbers, chars, strings (both for shsl strings and symbol names))
// is copied into the returned object,
// so changes to the object won't change the original data and viceversa
// 
// this, although inefficient, ensures that all the data contained in shsl
// objects can be safely managed by our autoamtic memory management
// and prevents weird data races we may encounter at the language boundary
shsl_ref shsl_mkint(long l) {
    return_mallocd_obj(.ref_count = 0, .type = SHSL_INT, .i = l);
}
shsl_ref shsl_mkreal(double d) {
    return_mallocd_obj(.ref_count = 0, .type = SHSL_REAL, .r = d);
}
shsl_ref shsl_mkstr(const char* str) {
    size_t len = strlen(str);
    char* c = (char*)calloc(len+1, sizeof(char));
    strcpy(c, str);
    return_mallocd_obj(.ref_count = 0, .type = SHSL_STR, .str = c);
}
shsl_ref shsl_mksym(const char* name) {
    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_SYM,
		       .sym = (shsl_sym){
			   .name = shsl_ref_add(shsl_mkstr(name)),
		       });
}
// the nocopy versions are for when you have the name allocated on the heap
// no one else is gonna touch it
// might as well take it
shsl_ref shsl_mkstr_nocopy(char* str) {
    return_mallocd_obj(.ref_count = 0, .type = SHSL_STR, .str = str);
}
shsl_ref shsl_mksym_nocopy(char* name) {
    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_SYM,
		       .sym = (shsl_sym){
			   .name = shsl_ref_add(shsl_mkstr_nocopy(name)),
		       });
}

#define SHSL_ERR_MAX_LENGTH 1024
shsl_ref shsl_vmkerr(shsl_ref data, const char* msg, va_list args) {
    static char buf[SHSL_ERR_MAX_LENGTH] = {0};
    vsprintf(buf, msg, args);

#ifdef SHSL_LOG_ERROR
    fprintf(stderr, "[ERROR] %s\n", buf);
    fprintf(stderr, "[ERROR] with data: ");
    shsl_fpr(data, stderr);
    fputc('\n', stderr);
#endif

    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_ERR,
		       .err = (shsl_err) {
			   .data = shsl_ref_add(data),
			   .msg = shsl_ref_add(shsl_mkstr(buf)),
		       });
}
shsl_ref shsl_mkerr(shsl_ref data, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    shsl_ref err = shsl_vmkerr(data, msg, args);
    va_end(args);
    return err;
}

shsl_ref shsl_mkcons(shsl_ref car, shsl_ref cdr) {
    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_CONS,
		       .cons = (shsl_cons) { .car = shsl_ref_add(car),
					     .cdr = shsl_ref_add(cdr), }
		      );
}
shsl_ref shsl_mkvec(size_t initial_capacity) {
    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_VEC,
		       .vec = (shsl_vec) {
			   .buf = (shsl_ref*)calloc(initial_capacity,
                                                    sizeof(shsl_ref)),
			   .size = 0,
			   .capacity = initial_capacity,
		       },
		      );
}
shsl_ref shsl_mkmap(size_t initial_capacity) {
    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_MAP,
		       .map = (shsl_map){
			   .buf = (shsl_kv*)calloc(initial_capacity, sizeof(shsl_kv)),
			   .size = 0,
			   .capacity = initial_capacity,
		       }
		      );
}
shsl_ref shsl_mkbuiltin_fn(shsl_ref env,
                           shsl_ref(*apply)(shsl_ref args,
                                            shsl_ref env)) {
    assert(shsl_is_nil(env)
	   || (shsl_is_cons(env) && shsl_is_map(env.ptr->cons.car))
	   && "if function env is not null it must be a list of maps!");
    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_BUILTIN_FN,
		       .builtin_fn = (shsl_builtin_fn) {
			   .env = shsl_ref_add(env),
			   .apply = apply,
		       }
		      );
}
shsl_ref shsl_mkbuiltin_macro(shsl_ref env,
                              shsl_ref(*expand)(shsl_ref args,
                                                shsl_ref env)) {
    assert(shsl_is_nil(env)
	   || (shsl_is_cons(env) && shsl_is_map(env.ptr->cons.car))
	   && "if macro env is not null it must be a list of maps!");

    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_BUILTIN_MACRO,
		       .builtin_macro = (shsl_builtin_fn) {
			   .env = shsl_ref_add(env),
			   .apply = expand,
		       }
		      );
}
shsl_ref shsl_mkuser_fn(shsl_ref env, shsl_lambda_list* lambda_list,
                        struct shsl_expr** body, size_t body_length) {
    assert(shsl_is_nil(env)
	   || (shsl_is_cons(env) && shsl_is_map(env.ptr->cons.car))
	   && "if function env is not null it must be a list of maps!");
    assert(body && "function body cannot be null pointer!");

    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_USER_FN,
		       .user_fn = (shsl_user_fn) {
			   .env = shsl_ref_add(env),
			   .lambda_list = lambda_list,
			   .body = body,
			   .body_length = body_length,
		       }
		      );
}
shsl_ref shsl_mkuser_macro(shsl_ref env, shsl_lambda_list* lambda_list,
                           struct shsl_expr** body, size_t body_length) {
    assert(shsl_is_nil(env)
	   || (shsl_is_cons(env) && shsl_is_map(env.ptr->cons.car))
	   && "if function env is not null it must be a list of maps!");
    assert(body && "macro body cannot be null pointer!");

    return_mallocd_obj(.ref_count = 0,
		       .type = SHSL_USER_MACRO,
		       .user_macro = (shsl_user_fn) {
			   .env = shsl_ref_add(env),
			   .lambda_list = lambda_list,
			   .body = body,
			   .body_length = body_length,
		       }
		      );
}

/// DATA OPERATIONS DEFINITIONS
/// GENERIC OPERATIONS DEFINITIONS
// all C string are copied by shsl_mk* constructors so there is no need to copy them
// in this function, the constructors will do their thing for copying the data
shsl_ref shsl_copy(shsl_ref ref) {
    switch(shsl_type(ref)) {
    case SHSL_NIL:    return shsl_ref_to_nil();
    case SHSL_SYM:    return shsl_mksym(ref.ptr->sym.name.ptr->str);
    case SHSL_INT:    return shsl_mkint(ref.ptr->i);
    case SHSL_REAL:   return shsl_mkint(ref.ptr->r);
    case SHSL_STR:    return shsl_mkstr(ref.ptr->str);

    case SHSL_ERR:    return shsl_mkerr(shsl_copy(ref.ptr->err.data),
                                        ref.ptr->err.msg.ptr->str);

    case SHSL_CONS:   return shsl_mkcons(shsl_copy(ref.ptr->cons.car),
                                         shsl_copy(ref.ptr->cons.cdr));
						      
    case SHSL_VEC: {
	shsl_ref copy = shsl_mkvec(ref.ptr->vec.size);
	shsl_vec_foreach(i, elt, ref)
	    shsl_vec_push(copy, shsl_copy(elt));
	return copy;
    }
    case SHSL_MAP: {
	shsl_ref copy = shsl_mkmap(ref.ptr->map.size);
	for(size_t i = 0; i<ref.ptr->map.size; ++i) {
	    shsl_map_set(copy,
			 shsl_copy(ref.ptr->map.buf[i].k),
			 shsl_copy(ref.ptr->map.buf[i].v));
	}
	return copy;
    }

    case SHSL_BUILTIN_FN:
    case SHSL_USER_FN:
    case SHSL_BUILTIN_MACRO:
    case SHSL_USER_MACRO:
	fprintf(stderr,
                "[SHSL WARNING] function object not copyable!\n");
	return shsl_ref_to_nil();
    }
    assert(0 && "unreachable");
}
void shsl_free(shsl_ref ref) {
#ifdef SHSL_LOG_GC
    fprintf(stdout, "[SHSL GC] freeing object %p\n", (void*)(ref.ptr));
    shsl_fpr(ref, stdout);
    fputc('\n', stdout);
#endif
    switch(shsl_type(ref)) {
    case SHSL_NIL:
	// fprintf(stderr, "cannot free NIL! You fucked something up!\n");
	break;

    case SHSL_STR:
	free(ref.ptr->str);
	free(ref.ptr);
	break;

    case SHSL_SYM:
	shsl_ref_drop(ref.ptr->sym.name);
	free(ref.ptr);
	break;

    case SHSL_ERR:
	shsl_ref_drop(ref.ptr->err.msg);
	shsl_ref_drop(ref.ptr->err.data);
	free(ref.ptr);
	break;
    case SHSL_CONS:
	shsl_ref_drop(ref.ptr->cons.car);
	shsl_ref_drop(ref.ptr->cons.cdr);
	free(ref.ptr);
	break;
    case SHSL_VEC:
	for(size_t i = 0; i<ref.ptr->vec.size; ++i)
	    shsl_ref_drop(ref.ptr->vec.buf[i]);
	free(ref.ptr->vec.buf);
	free(ref.ptr);
	break;
    case SHSL_MAP:
	for(size_t i = 0; i<ref.ptr->map.size; ++i) {
	    shsl_ref_drop(ref.ptr->map.buf[i].k);
	    shsl_ref_drop(ref.ptr->map.buf[i].v);
	}
	free(ref.ptr->map.buf);
	free(ref.ptr);
	break;

    case SHSL_BUILTIN_FN:
	shsl_ref_drop(ref.ptr->builtin_fn.env);
	free(ref.ptr);
	break;
    case SHSL_USER_FN:
	shsl_ref_drop(ref.ptr->user_fn.env);
        shsl_ref_drop(ref.ptr->user_fn.lambda_list->all);
        shsl_ref_drop(ref.ptr->user_fn.lambda_list->variadic);
        shsl_ref_drop(ref.ptr->user_fn.lambda_list->keyword);
        free(ref.ptr->user_fn.lambda_list);

	for(size_t i = 0; i<ref.ptr->user_fn.body_length; ++i)
	    shsl_expr_free(ref.ptr->user_fn.body[i]);
        free(ref.ptr->user_fn.body);

	free(ref.ptr);
	break;
    case SHSL_BUILTIN_MACRO:
	shsl_ref_drop(ref.ptr->builtin_macro.env);
	free(ref.ptr);
	break;
    case SHSL_USER_MACRO:
	shsl_ref_drop(ref.ptr->user_macro.env);
        shsl_ref_drop(ref.ptr->user_macro.lambda_list->all);
        shsl_ref_drop(ref.ptr->user_macro.lambda_list->variadic);
        shsl_ref_drop(ref.ptr->user_macro.lambda_list->keyword);
        free(ref.ptr->user_macro.lambda_list);

	for(size_t i = 0; i<ref.ptr->user_macro.body_length; ++i)
	    shsl_expr_free(ref.ptr->user_macro.body[i]);
        free(ref.ptr->user_macro.body);

	free(ref.ptr);
	break;

    default:
	free(ref.ptr);
	break;
    }

#ifdef SHSL_LOG_GC
    fprintf(stdout, "[SHSL GC] freed object %p\n", (void*)(ref.ptr));
#endif    
}
bool shsl_eq(shsl_ref lhs_ref, shsl_ref rhs_ref) {
    const shsl_obj* const lhs = lhs_ref.ptr;
    const shsl_obj* const rhs = rhs_ref.ptr;

    if(lhs->type != rhs->type)
	return false;
	
    switch(lhs->type) {
    case SHSL_NIL:
	return true;
    case SHSL_SYM:
	return strcmp(lhs->sym.name.ptr->str, rhs->sym.name.ptr->str) == 0;
    case SHSL_ERR:
	return strcmp(lhs->err.msg.ptr->str, rhs->err.msg.ptr->str) == 0
	    && shsl_eq(lhs->err.data, lhs->err.data);

    case SHSL_INT:
	return lhs->i == rhs->i;
    case SHSL_REAL:
	return lhs->r == rhs->r;
    case SHSL_STR:
	return strcmp(lhs->str, rhs->str) == 0;

    case SHSL_CONS:
	return shsl_eq(lhs->cons.car, rhs->cons.car)
	    && shsl_eq(lhs->cons.cdr, rhs->cons.cdr);

    case SHSL_VEC:
	if(lhs->vec.size != rhs->vec.size)
	    return false;
	for(size_t i = 0; i<lhs->vec.size; ++i)
	    if(!shsl_eq(lhs->vec.buf[i], rhs->vec.buf[i]))
		return false;
	return true;

    case SHSL_MAP:
	if(lhs->map.size != rhs->map.size)
	    return false;
	for(size_t i = 0; i<lhs->map.size; ++i)
	    if(!shsl_eq(shsl_map_get(lhs_ref, lhs->map.buf[i].k),
			shsl_map_get(rhs_ref, lhs->map.buf[i].k)))
		return false;
	return true;
    case SHSL_BUILTIN_FN:
    case SHSL_USER_FN:
    case SHSL_BUILTIN_MACRO:
    case SHSL_USER_MACRO:
	fprintf(stderr, "TODO: this comparison's not implemeneted yet!\n");
	return false;
    }
    assert(0 && "unreachable");
}

/// CONS MANIPULATIONS DEFINITIONS
void shsl_set_car(shsl_ref cons_ref, shsl_ref new_car) {
    assert(shsl_is_cons(cons_ref));
    shsl_ref_set(&(cons_ref.ptr->cons.car), new_car);
}
void shsl_set_cdr(shsl_ref cons_ref, shsl_ref new_cdr) {
    assert(shsl_is_cons(cons_ref));
    shsl_ref_set(&(cons_ref.ptr->cons.cdr), new_cdr);
}

/// VECTOR MANIPULATIONS DEFINITIONS
void shsl_vec_expand(shsl_ref vec_ref, size_t new_size) {
    assert(shsl_is_vec(vec_ref));

    if(vec_ref.ptr->vec.capacity >= new_size) return;
    vec_ref.ptr->vec.buf = (shsl_ref*)reallocarray
        (vec_ref.ptr->vec.buf, new_size, sizeof(shsl_ref));
    vec_ref.ptr->vec.capacity = new_size;
}
void shsl_vec_push(shsl_ref vec_ref, shsl_ref obj) {
    assert(shsl_is_vec(vec_ref));

    if(vec_ref.ptr->vec.size == vec_ref.ptr->vec.capacity)
	shsl_vec_expand(vec_ref,
			vec_ref.ptr->vec.capacity
			+ (vec_ref.ptr->vec.capacity/2)
			+ 1);
    // append
    shsl_ref_add(obj);
    vec_ref.ptr->vec.buf[vec_ref.ptr->vec.size] = obj;
    vec_ref.ptr->vec.size++;
}
shsl_ref shsl_vec_get(shsl_ref vec_ref, size_t i) {
    assert(shsl_is_vec(vec_ref));

    if(i >= vec_ref.ptr->vec.size)
	return shsl_mkerr(shsl_ref_to_nil(), "out of bounds array read!");
    return vec_ref.ptr->vec.buf[i];
}
void shsl_vec_set(shsl_ref vec_ref, size_t i, shsl_ref new_val) {
    assert(shsl_is_vec(vec_ref));

    // TODO: gestione degli errori un po' di più magari
    if(i >= vec_ref.ptr->vec.size)
	fprintf(stderr, "out of bounds array write!\n");

    shsl_ref_set(&vec_ref.ptr->vec.buf[i], new_val);
}
size_t shsl_vec_length(shsl_ref vec_ref) {
    assert(shsl_is_vec(vec_ref));
    return vec_ref.ptr->vec.size;
}
shsl_ref shsl_vec_first(shsl_ref vec_obj) {
    assert(shsl_is_vec(vec_obj));
    if(shsl_vec_length(vec_obj) == 0)
        return shsl_mkerr(vec_obj, "cannot get first element of empty array!");
    return shsl_vec_get(vec_obj, 0);
}
shsl_ref shsl_vec_last(shsl_ref vec_obj) {
    assert(shsl_is_vec(vec_obj));
    return shsl_vec_get(vec_obj, shsl_vec_length(vec_obj)-1);
}
// make a different copy of size and capacity, but keep the
// underlying buffer
// useful to make transient slices
shsl_ref shsl_vec_shallow_copy(shsl_ref orig) {
    return_mallocd_obj(
                       .ref_count = 0,
                       .type = SHSL_VEC,
                       .vec = (shsl_vec) {
                           .buf = orig.ptr->vec.buf,
                           .size = orig.ptr->vec.size,
                           .capacity = orig.ptr->vec.capacity,
                       },
                      );
}
shsl_ref shsl_vec_slice(shsl_ref vec_obj, size_t first, size_t last) {
    assert(shsl_is_vec(vec_obj));
    if(first > last)
        return shsl_mkerr(vec_obj,
                          "tried creating vector slice with beginning index "
                          "greater than ending index! (begin %zu, end %zu)",
                          first, last);
    
    size_t slice_size = last-first;
    shsl_ref slice = shsl_mkvec(slice_size);
    for(size_t i = first; i<last; ++i)
        shsl_vec_push(slice, shsl_vec_get(vec_obj, i));
    slice.ptr->vec.size = slice_size;
    return slice;
}

/// MAP MANIPULATIONS DEFINITIONS
void shsl_map_expand(shsl_ref map_ref, size_t new_size) {
    assert(shsl_is_map(map_ref));

    if (map_ref.ptr->map.capacity >= new_size) return;
    map_ref.ptr->map.buf = (shsl_kv*)reallocarray
        (map_ref.ptr->map.buf, new_size, sizeof(shsl_kv));
    map_ref.ptr->map.capacity = new_size;
}
ssize_t shsl_map_index(shsl_ref map_ref, shsl_ref key) {
    assert(shsl_is_map(map_ref));

    for(size_t i = 0; i<map_ref.ptr->map.size; ++i)
	if(shsl_eq(key, map_ref.ptr->map.buf[i].k))
	    return (ssize_t)i;
    return -1;
}
shsl_ref shsl_map_get(shsl_ref map_ref, shsl_ref key) {
    assert(shsl_is_map(map_ref));

    ssize_t i = shsl_map_index(map_ref, key);
    if(i>=0) return map_ref.ptr->map.buf[i].v;
    return shsl_ref_to_nil();
}
void shsl_map_set(shsl_ref map_ref, shsl_ref key, shsl_ref new_val) {
    assert(shsl_is_map(map_ref));

    ssize_t i = shsl_map_index(map_ref, key);
    if(i>=0) {
        shsl_ref_set(&(map_ref.ptr->map.buf[i].v), new_val);
    }
    else {
	if(map_ref.ptr->map.size == map_ref.ptr->map.capacity)
	    shsl_map_expand(map_ref,
			    map_ref.ptr->vec.capacity
			    + (map_ref.ptr->vec.capacity/2)
			    + 1);
	shsl_ref_add(key);
	shsl_ref_add(new_val);
	map_ref.ptr->map.buf[map_ref.ptr->map.size] = (shsl_kv) {
	    .k = key,
	    .v = new_val,
	};
	map_ref.ptr->map.size++;
    }
}

/// COLLECTION BUILDERS DEFINITIONS
typedef struct shsl_cons_builder {
    shsl_ref first; shsl_ref last;
} shsl_cons_builder;
typedef struct shsl_vec_builder {
    shsl_ref vec;
} shsl_vec_builder;
typedef struct shsl_map_builder {
    shsl_ref map; shsl_ref curr_key; bool reading_key;
} shsl_map_builder;

typedef struct shsl_cb {
    SHSL_CB_TYPE type;
    union {
	shsl_cons_builder cons_builder;
	shsl_vec_builder vec_builder;
	shsl_map_builder map_builder;
    };
} shsl_cb;
shsl_cb shsl_cb_make(SHSL_CB_TYPE type) {
    switch(type) {
    case SHSL_CB_LIST:
	return (shsl_cb) {
	    .type = SHSL_CB_LIST,
	    .cons_builder = (shsl_cons_builder) {
		.first = shsl_ref_to_nil(),
		.last = shsl_ref_to_nil(),
	    }
	};
    case SHSL_CB_VEC:
	return (shsl_cb) {
	    .type = SHSL_CB_VEC,
	    .vec_builder = (shsl_vec_builder){
		.vec = shsl_mkvec(1),
	    }
	};
    case SHSL_CB_MAP:
	return (shsl_cb) {
	    .type = SHSL_CB_MAP,
	    .map_builder = (shsl_map_builder) {
		.map = shsl_mkmap(1),
		.curr_key = shsl_ref_to_nil(),
		.reading_key = true,
	    }
	};
    }
    assert(0 && "unreachable");
}
void shsl_cb_add(shsl_cb* cb, shsl_ref ref) {
    switch(cb->type) {
    case SHSL_CB_LIST: 
	if(shsl_is_nil(cb->cons_builder.first)) {
	    cb->cons_builder.first = shsl_mkcons(ref, shsl_ref_to_nil());
	    cb->cons_builder.last = cb->cons_builder.first;
	}
	else { 
	    shsl_set_cdr
		(cb->cons_builder.last,
		 shsl_mkcons(ref, shsl_ref_to_nil()));
	    cb->cons_builder.last.ptr = cb->cons_builder.last.ptr->cons.cdr.ptr;
	}
	break;
    case SHSL_CB_VEC: 
	shsl_vec_push(cb->vec_builder.vec, ref);
	break;
    case SHSL_CB_MAP: 
	if(cb->map_builder.reading_key) {
	    cb->map_builder.curr_key = ref;
	    cb->map_builder.reading_key = false;
	}
	else {
	    shsl_map_set
		(cb->map_builder.map,
		 cb->map_builder.curr_key,
		 ref);
	    cb->map_builder.reading_key = true;
	}
    }
}
shsl_ref shsl_cb_get(shsl_cb cb) {
    switch (cb.type) {
    case SHSL_CB_LIST:
	return cb.cons_builder.first;
    case SHSL_CB_VEC:
	return cb.vec_builder.vec;
    case SHSL_CB_MAP:
	if(cb.map_builder.reading_key)
	    return cb.map_builder.map;
	else
	    return shsl_mkerr
		(cb.map_builder.map,
		 "tried building map with with odd number of elements!");
    }
    assert(0 && "unreachable");
}

/// LIST OPERATIONS DEFINITIONS
shsl_ref shsl_car(shsl_ref ref) {
    switch(shsl_type(ref)) {
    case SHSL_NIL:
	return shsl_ref_to_nil();
    case SHSL_CONS:
	return ref.ptr->cons.car;
    default:
	return shsl_mkerr(ref, "CAR: cannot get car as object is not a list");
    }
}
shsl_ref shsl_cdr(shsl_ref ref) {
    switch(shsl_type(ref)) {
    case SHSL_NIL:
	return shsl_ref_to_nil();
    case SHSL_CONS:
	return ref.ptr->cons.cdr;
    default:
	return shsl_mkerr(ref, "CDR: cannot get car as object is not a list");
    }
}
shsl_ref shsl_nthcdr(shsl_ref ref, size_t n) {
    while(n-- && !shsl_is_err(ref))
	ref = shsl_cdr(ref);
    if(shsl_is_err(ref))
	return shsl_mkerr(ref, "NTHCDR: cannot get nthcdr of list :(");
    return ref;
}
shsl_ref shsl_nth(shsl_ref list_ref, size_t n) {
    shsl_ref nth = shsl_car(shsl_nthcdr(list_ref, n));
    if(shsl_is_err(nth))
	return shsl_mkerr(nth, "NTH: can't get nth element of object");
    return nth;
}
ssize_t shsl_list_length(shsl_ref list_ref) {
    for(ssize_t i = 0; ; ++i) {
	switch(shsl_type(list_ref)) {
	case SHSL_NIL:
	    return i;
	case SHSL_CONS:
	    list_ref = list_ref.ptr->cons.cdr;
	    break;
	default:
	    return -1;
	}
    }
}
char* shsl_sym_name(shsl_ref sym_ref) {
    // TODO: copy
    return sym_ref.ptr->sym.name.ptr->str;
}
long shsl_int(shsl_ref ref) {
    assert(shsl_type(ref) == SHSL_INT);
    return ref.ptr->i;
}
double shsl_real(shsl_ref ref) {
    assert(shsl_is_int(ref) || shsl_is_real(ref));
    return shsl_is_int(ref)?(double)ref.ptr->i:ref.ptr->r;
}

/// FUNCTION OPERATIONS DEFINITIONS
shsl_ref shsl_fn_env(shsl_ref ref) {
    assert(shsl_is_fn(ref));
    switch(ref.ptr->type) {
    case SHSL_BUILTIN_FN:
	return ref.ptr->builtin_fn.env;
    case SHSL_USER_FN:
	return ref.ptr->user_fn.env;
    case SHSL_BUILTIN_MACRO:
	return ref.ptr->builtin_macro.env;
    case SHSL_USER_MACRO:
	return ref.ptr->user_macro.env;
    default:
	assert(0 && "unreachable");
    }
}
void shsl_fn_env_mark_weak(shsl_ref ref) {
    assert(shsl_is_fn(ref));
    switch(ref.ptr->type) {
    case SHSL_BUILTIN_FN:
	shsl_ref_mark_weak(&(ref.ptr->builtin_fn.env));
	break;
    case SHSL_USER_FN:
	shsl_ref_mark_weak(&(ref.ptr->user_fn.env));
	break;
    case SHSL_BUILTIN_MACRO:
	shsl_ref_mark_weak(&(ref.ptr->builtin_macro.env));
	break;
    case SHSL_USER_MACRO:
	shsl_ref_mark_weak(&(ref.ptr->user_macro.env));
	break;
    default:
	assert(0 && "unreachable");
    }
}


//// LEXER DEFINITIONS
//// ----------------------------------------------------------------------------
typedef struct shsl_token {
    SHSL_TOKEN_TYPE type;
    shsl_ref ref;
} shsl_token;
// pair of read token/remaining string after token
// so we can lex a whole string going like:
// read a token, get the rest of the string after the token
// pass the remaining string to the lexer, and on and on again until EOF

// TODO: update into a "lexer state" or something like that also keeping track of
// line number, column number, position, and file name
// for better error reporting
// this would also imply having to change lexer implementation and signatures a bunch
// since, for instance, token_off would likely become a token_advance receiving 
// an initial lexer state, and you'd pass the starting string not to token_off as is 
// but to the some shsl_lexer_initial_state(filename, str); for instance
typedef struct lexer_pair {
    shsl_token token;
    char* remaining;
} lexer_pair;

// for a lot of token types (parens, quotes, et al) we only need to know the type
// of the token and don't really need the object the token carries,
// return a token with that type and no underlying object that we care about
shsl_token empty_token(SHSL_TOKEN_TYPE token_type) {
    return (shsl_token) {
	.type = token_type,
	.ref = shsl_ref_to_nil(),
    };
}
// since we return errors as special token pairs, might as well have an
// ad hoc function for that
lexer_pair error_lexer_pair(const char* errmsg) {
    return (lexer_pair) {
	.token = (shsl_token) {
	    .type = SHSL_TOK_ERROR,
	    .ref = shsl_mkerr(shsl_ref_to_nil(), "[SHSL LEXER] ERROR: %s", errmsg),
	},
	.remaining = NULL,
    };
}

// special characters are chars that once encountered you go like
// "ok, I'm done with the current thing, this character starts the next thing"
bool is_special_char(char c) {
    const char special[] = {'(', ')',
			    '[', ']',
			    '{', '}',
			    '\\', ',', '`', '\'', '"', '\0'};
    const char* s = &special[0];
    while(*s!='\0' && *s!=c) s++;
    return *s!='\0';
}
// character that can be part of a symbol (or number)
bool is_symbol_char(char c) {
    return isprint(c) && (!isspace(c)) && (!is_special_char(c));
}
// try parsing from c to c+len as an integer
// if success, return true and write the result into the *into pointer
// if failure (it wasn't an integer), return false and leave *into unaltered
bool try_parse_integer(char* c, size_t len, long* into) {
    long acc = 0;
    bool neg = false;
    if(len == 1 && (*c == '-' || *c == '+')) return false;
    if(*c=='+') {
	len--;
	c++;
    }
    if(*c=='-') {
	neg = true;
	len--;
	c++;
    }
    while(len--) {
	if(*c > '9' || *c < '0')
	    return false;
	acc = (acc*10) + (*c-'0');
	c++;
    }
    if(neg) acc = -acc;
    *into = acc;
    return true;
}
// copy characters from c to c+len into a fresh null terminated string
char* slice_to_fresh_str(const char* c, size_t len) {
    char* fresh = (char*)calloc(len+1, sizeof(char));
    memcpy(fresh, c, len);
    fresh[len] = '\0';
    return fresh;
}
// parse a non special token starting at c
// a non special token is a token not made of/delimited by special chars
// can be either symbol or a number
// if from c onwards is a valid integer, returns an integer token
// otherwise same with floats
// TODO: parsing floats
// otherwise returns a symbol, if the symbol is nil it returns a nil token
shsl_token parse_non_special_token(char*c, size_t len) {
    long l;
    if (try_parse_integer(c, len, &l))
	return (shsl_token) {
	    .type = SHSL_TOK_INTEGER,
	    .ref = shsl_mkint(l),
	};

    if (len == 3 && c[0] == 'n' && c[1] == 'i' && c[2] == 'l')
	return (shsl_token) {
	    .type = SHSL_TOK_NIL,
	    .ref = shsl_ref_to_nil(),
	};
	
    shsl_token t =  (shsl_token) {
	.type = SHSL_TOK_SYMBOL,
	.ref = shsl_mksym_nocopy(slice_to_fresh_str(c, len)),
    };
    return t;
}

// read a token off of string and return the pair of that token plus
// the string after the token
lexer_pair token_off(char* str) {
    // handle null pointer string
    if(!str)
	return error_lexer_pair("cannot read null pointer to string!");

    // skip whitespace
    while(isspace(*str)) str++;

    // handle empty string (we reached the null terminator)
    // once we read SHSL_TOK_EOF we don't need to read any more chars, so we 
    // purposefully return an invalid string to ensure that
    if(*str == '\0')
	return (lexer_pair) {
	    .token = empty_token(SHSL_TOK_EOF),
	    .remaining = NULL,
	};

    // handle special chars
    switch(*str) {
	// comments
    case ';':
        // make sure to handle end of string as well as end of line
        while(*str != '\n' && *str != '\0') str++;
        if(*str == '\n') str++;
        return token_off(str);

	// parentheses
    case '(':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_OPEN_PAREN),
			     .remaining = str+1, };
    case ')':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_CLOSE_PAREN),
			     .remaining = str+1, };
    case '[':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_OPEN_SQUARE),
			     .remaining = str+1, };
    case ']':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_CLOSE_SQUARE),
			     .remaining = str+1, };
    case '{':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_OPEN_CURLY),
			     .remaining = str+1, };
    case '}':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_CLOSE_CURLY),
			     .remaining = str+1, };

	// quotes and quasiquotes 
    case '\'':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_QUOTE),
			     .remaining = str+1, };
    case '`':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_QUASIQUOTE),
			     .remaining = str+1, };
    case ',':
	return (lexer_pair){ .token = empty_token(SHSL_TOK_COMMA),
			     .remaining = str+1, };

    case '"': {
        shsl_string_builder sb = {0};
        char* iter = str+1;
        while(*iter != '\0' && *iter != '"') {
            if(*iter == '\\') {
                char escape = *(iter+1);
                if(escape == '\0') {
                    if(sb.size != 0) free(sb.buf);
                    return error_lexer_pair
                        ("why did try and use the null terminator "
                         "in an escape sequence? how do you even do that?");
                }
                if(!does_char_escape(escape)) {
                    if(sb.size != 0) free(sb.buf);
                    return error_lexer_pair
                        (shsl_append_chars("unrecognized escape sequence: \\",
                                           escape, '!'));
                }
                shsl_sb_push(&sb, escape_to_escaped(escape));
                iter+=2;
            }
            else {
                shsl_sb_push(&sb, *iter);
                iter++;
            }
        }
        if(iter == str+1) {
            // that is, if we didn't advance for shit
            return (lexer_pair) {
                .token = (shsl_token) {
                    .type = SHSL_TOK_STRING,
                    .ref = shsl_mkstr(""),
                },
                .remaining = iter+1, // skip final delimiting '"'
            };
        }
        else if(*iter=='\0') {
            // unterminated string literal
            if(sb.size != 0) free(sb.buf);
            return error_lexer_pair("unterminated string literal!");
        }
        else {
            // string closed correctly
            assert(*iter == '"');
            shsl_sb_push(&sb, '\0');
            return (lexer_pair) {
                .token = (shsl_token) {
                    .type = SHSL_TOK_STRING,
                    .ref = shsl_mkstr_nocopy(shsl_sb_get(&sb)),
                },
                .remaining = iter+1, // skip final delimiting '"'
            };
        }
    }
    }

    // if we got here then it's not a special char
    // it's either a symbol or a number
    char* c = str;
    while(is_symbol_char(*c)) c++;

    return (lexer_pair){
        .token = parse_non_special_token(str, c-str),
        .remaining = c,
    };
}

//// PARSER DEFINITIONS
//// ----------------------------------------------------------------------------

// same logic as lexer_pair
// TODO: update into parser state?
typedef struct parser_pair {
    shsl_ref ref;
    char* remaining;
} parser_pair;
parser_pair parse_off(char* str) {
    lexer_pair lp = token_off(str);
    switch(lp.token.type) {
        // handle literals
    case SHSL_TOK_NIL:
    case SHSL_TOK_SYMBOL:
    case SHSL_TOK_INTEGER:
    case SHSL_TOK_REAL:
    case SHSL_TOK_STRING:
        return (parser_pair) {
            .ref = lp.token.ref,
            .remaining = lp.remaining,
        };

        // handle parens        
    case SHSL_TOK_OPEN_PAREN:
        return parse_until(lp.remaining,
                           shsl_cb_make(SHSL_CB_LIST),
                           SHSL_TOK_CLOSE_PAREN,
                           (SHSL_TOKEN_TYPE[]){SHSL_TOK_CLOSE_SQUARE,
                                               SHSL_TOK_CLOSE_CURLY},
                           2);
    case SHSL_TOK_OPEN_SQUARE:
        return parse_until(lp.remaining,
                           shsl_cb_make(SHSL_CB_VEC),
                           SHSL_TOK_CLOSE_SQUARE,
                           (SHSL_TOKEN_TYPE[]){SHSL_TOK_CLOSE_PAREN,
                                               SHSL_TOK_CLOSE_CURLY},
                           2);
    case SHSL_TOK_OPEN_CURLY:
        return parse_until(lp.remaining,
                           shsl_cb_make(SHSL_CB_MAP),
                           SHSL_TOK_CLOSE_CURLY,
                           (SHSL_TOKEN_TYPE[]){SHSL_TOK_CLOSE_PAREN,
                                               SHSL_TOK_CLOSE_SQUARE},
                           2);
        
        // handle quotes
    case SHSL_TOK_QUOTE: {
        parser_pair pp = parse_off(lp.remaining);
        return (parser_pair) {
            .ref = shsl_mkcons(shsl_mksym("quote"),
                               shsl_mkcons(pp.ref, shsl_ref_to_nil())),
            .remaining = pp.remaining,
        };
    }
    case  SHSL_TOK_QUASIQUOTE: {
        parser_pair pp = parse_off(lp.remaining);
        return (parser_pair) {
            .ref = shsl_mkcons(shsl_mksym("quasiquote"),
                               shsl_mkcons(pp.ref, shsl_ref_to_nil())),
            .remaining = pp.remaining,
        };
    }
    case  SHSL_TOK_COMMA: {
        parser_pair pp = parse_off(lp.remaining);
        return (parser_pair) {
            .ref = shsl_mkcons(shsl_mksym("comma"),
                               shsl_mkcons(pp.ref, shsl_ref_to_nil())),
            .remaining = pp.remaining,
        };
    }

        // TODO: better error/finish return
    case SHSL_TOK_CLOSE_PAREN:
        return (parser_pair) {
            .ref = shsl_mkerr (shsl_ref_to_nil(),
                               "[SHSL PARSER] ERROR: "
                               "unmatched close parentheses!"),
            .remaining = NULL,
        };
    case SHSL_TOK_CLOSE_SQUARE:
        return (parser_pair) {
            .ref = shsl_mkerr (shsl_ref_to_nil(),
                               "[SHSL PARSER] ERROR: "
                               "unmatched close square bracket!"),
            .remaining = NULL,
        };
    case SHSL_TOK_CLOSE_CURLY:
        return (parser_pair) {
            .ref = shsl_mkerr (shsl_ref_to_nil(),
                               "[SHSL PARSER] ERROR: "
                               "unmatched close curly bracket!"),
            .remaining = NULL,
        };
    case SHSL_TOK_ERROR:
        return (parser_pair) {
            .ref = shsl_mkerr (lp.token.ref,
                               "[SHSL PARSER] ERROR: "
                               "cannot proceed further, received error "
                               "from lexer!"),
            .remaining = NULL,
        };

    case SHSL_TOK_EOF:
        return ((parser_pair){0});
    }
    assert(0 && "unreachable");
}
parser_pair parse_until(char* str,
                        shsl_cb cb,
                        SHSL_TOKEN_TYPE stop,
                        SHSL_TOKEN_TYPE* error_on, size_t error_on_length) {
    // we only use this lexer_pair to peek the next token
    // if the next token is our stop token, we stop
    // if it's one of our error tokens, we error
    // otherwise we parse the next object on until we reach a stop token
    while(true) {
        lexer_pair lp = token_off(str);
        if(lp.token.type == stop) {
            if(!shsl_is_nil(lp.token.ref))
                shsl_free(lp.token.ref);
            return (parser_pair) {
                .ref = shsl_cb_get(cb),
                .remaining = lp.remaining,
            };
        }

        for(size_t i = 0; i<error_on_length; ++i)
            if(lp.token.type == error_on[i]) {
                fprintf(stderr,
                        "error: open parentheses \"%s\" closed by \"%s\"\n",
                        stop == SHSL_TOK_CLOSE_PAREN ? "("
                        : stop == SHSL_TOK_CLOSE_SQUARE ? "["
                        : stop == SHSL_TOK_CLOSE_CURLY ? "{"
                        : "wtf bro",
                        error_on[i] == SHSL_TOK_CLOSE_PAREN ? ")"
                        : error_on[i] == SHSL_TOK_CLOSE_SQUARE ? "]"
                        : error_on[i] == SHSL_TOK_CLOSE_CURLY ? "}"
                        : "WTF BRO");
                return ((parser_pair){0});
            }

        // append parsed object to acc list
        parser_pair pp = parse_off(str);
        shsl_cb_add(&cb, pp.ref);
        str = pp.remaining;
        if(!shsl_is_nil(lp.token.ref))
            shsl_free(lp.token.ref);
    }
}

//// EVALUATOR DEFINITIONS
//// ----------------------------------------------------------------------------

/// EXPRESSION TYPE DEFINITIONS
// NOTE
// literals don't own the expression they're literals of, they hold a ref to it
// when deleting literals, only decrease the refcount of the ref
// (shit gets weird with symbols so...)
// that said, when evaluated literals will return a safe copy of the object they
// hold, this way if we set something to a literal then change it doesn't alter
// the literal
// 
// expressions do own their subexpressions tho
// when deleting an expression, delete its subexpressions as well
typedef struct shsl_vec_expr {
    shsl_expr** elts;
    size_t size;
} shsl_vec_expr;
typedef struct shsl_map_expr {
    shsl_expr** keys;
    shsl_expr** vals;
    size_t size;
} shsl_map_expr;
typedef struct shsl_if_expr {
    shsl_expr* condition;
    shsl_expr* then_part;
    shsl_expr* else_part;
} shsl_if_expr;
typedef struct shsl_do_expr {
    shsl_expr** body;
    size_t body_length;
} shsl_do_expr;
typedef struct shsl_do_poking_expr {
    shsl_expr** body;
    size_t body_length;
} shsl_do_poking_expr;
typedef struct shsl_let_expr {
    shsl_ref keys;
    shsl_expr** vals;
    size_t binds_length;
    shsl_expr** body;
    size_t body_length;
} shsl_let_expr;
typedef struct shsl_while_expr {
    shsl_expr* condition;
    shsl_expr** body;
    size_t body_length;
} shsl_while_expr;
typedef struct shsl_def_expr {
    shsl_ref name;           // must be symbol
    shsl_expr* value;
} shsl_def_expr;
typedef struct shsl_set_expr {
    shsl_ref name;           // must be symbol
    shsl_expr* value;
} shsl_set_expr;
typedef struct shsl_fn_expr {
    shsl_lambda_list* lambda_list;
    shsl_expr** body;
    size_t body_length;
} shsl_fn_expr;
typedef struct shsl_macro_expr {
    shsl_lambda_list* lambda_list;
    shsl_expr** body;
    size_t body_length;
} shsl_macro_expr;
typedef struct shsl_funcall_expr {
    shsl_expr* fn_expr;
    shsl_expr** args;
    size_t args_length;
} shsl_funcall_expr;

typedef struct shsl_expr {
    SHSL_EXPR_TYPE type;
    union {
        shsl_ref literal;
        shsl_ref lookup_symbol; // must be symbol

        shsl_vec_expr vec_expr;
        shsl_map_expr map_expr;

        shsl_if_expr if_expr;
        shsl_do_expr do_expr;
        shsl_do_poking_expr do_poking_expr;
        shsl_let_expr let_expr;
        shsl_while_expr while_expr;

        shsl_def_expr def_expr;
        shsl_set_expr set_expr;

        shsl_fn_expr fn_expr;
        shsl_macro_expr macro_expr;

        shsl_funcall_expr funcall_expr;
    };
} shsl_expr;

/// EXPRESSION FUNCTIONS DEFINITIONS
// https://stackoverflow.com/questions/6750512/gcc-warning-iso-c-does-not-permit-named-variadic-macros
#define return_mallocd_expr(...) do {                   \
        shsl_expr* expr =                               \
            (shsl_expr*)malloc(sizeof(shsl_expr));      \
        *expr = (shsl_expr){__VA_ARGS__};               \
        return expr;                                    \
    } while(0)
// this is how we represent parsing errors
bool shsl_expr_is_error(shsl_expr* expr) {
    return expr->type == SHSL_EXPR_LITERAL
        && expr->literal.ptr->type == SHSL_ERR;
}
// this is how we return parsing errors
shsl_expr* shsl_vexpr_error(shsl_ref form, const char* msg, va_list args) {
    shsl_ref err = shsl_vmkerr(form, msg, args);
#ifdef SHSL_LOG_ERROR_EXPR
    fprintf(stderr, "[SHSL PARSER ERROR] %s\n", msg);
    fprintf(stderr, "[SHSL PARSER ERROR] with data: ");
    shsl_fpr(form, stderr);
    fputc('\n', stderr);
#endif
    return_mallocd_expr(.type = SHSL_EXPR_LITERAL,
                        .literal = err);
}
shsl_expr* shsl_expr_error(shsl_ref form, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    shsl_expr* err_expr = shsl_vexpr_error(form, msg, args);
    va_end(args);

    return err_expr;
}
// this is how we propagate errors
// (return an error with a reference to the error that caused it)
shsl_expr* shsl_expr_further_error(shsl_expr* orig, const char* msg, ...) {
    assert(shsl_expr_is_error(orig));
    va_list args;
    va_start(args, msg);
    shsl_ref err = shsl_vmkerr(orig->literal, msg, args);
    va_end(args);
    return_mallocd_expr(.type = SHSL_EXPR_LITERAL,
                        .literal = err);
}

/// PARSING UTILITIES DEFINITIONS
shsl_expr** shsl_form_list_to_expr_arr(shsl_ref form, shsl_ref env) {
    // returns an array of forms parsed starting from form onwards until nil
    // ie: parses the whole subtree starting at form
    // if it encounters an error form while parsing it adds it to the array and
    // stops at the error, so you can grab the error then free everything that came
    // before the error

    // other than error handling basically acts as
    // (map 'vector #'form-to-expr form)
    assert(shsl_is_list(form));
    size_t len = shsl_list_length(form);
    shsl_expr** res = (shsl_expr**)calloc(len, sizeof(shsl_expr*));
    for(size_t i = 0; i<len; ++i) {
        res[i] = shsl_form_to_expr(shsl_nth(form, i), env);
    }
    return res;
}
ssize_t shsl_expr_arr_find_err(shsl_expr** arr, size_t len) {
    // returns index of first element in arr that is an error
    // if no error is found returns -1
    for(size_t i = 0; i<len; ++i) {
        if(shsl_expr_is_error(arr[i])) return (ssize_t)i;
    }
    return -1;
}
void shsl_free_expr_arr(shsl_expr** arr, size_t len) {
    // frees every expression in from arr to arr+len, then frees arr
    for(size_t i = 0; i<len; ++i) {
        shsl_expr_free(arr[i]);
    }
    free(arr);
}

/// FORM TRANSLATION FUNCTIONS DEFINITIONS
// (we pass an env to this because shsl_form_to_expr is also responsible for
//  macro expansion (it was easier to put it here than to make a separate
//  function for it, forms are far more malleable when they're still lists
//  and shit, calling macroexpand on expression would be a royal pain in the
//  ass, I tried))
shsl_expr* shsl_form_to_expr(shsl_ref form, shsl_ref env) {

    switch(form.ptr->type) {
    case SHSL_INT:
    case SHSL_REAL:
    case SHSL_STR:
    case SHSL_NIL:
        return_mallocd_expr(.type = SHSL_EXPR_LITERAL,
                            .literal = shsl_ref_add(form));

    case SHSL_SYM:
        return_mallocd_expr(.type = SHSL_EXPR_LOOKUP,
                            .lookup_symbol = shsl_ref_add(form));

    case SHSL_VEC: {
        shsl_expr** elt_exprs = (shsl_expr**)calloc(shsl_vec_length(form),
                                                    sizeof(shsl_expr*));

        // can't use shsl_form_list_to_expr_arr as vector forms aren't lists
        shsl_vec_foreach(i, form_elt, form) {
            shsl_expr* next = shsl_form_to_expr(form_elt, env);
            if(!shsl_expr_is_error(next)) {
                elt_exprs[i] = next;
            }
            else {
                for(size_t j = 0; j<i; ++j)
                    shsl_expr_free(elt_exprs[j]);
                free(elt_exprs);
                return shsl_expr_error
                    (form,
                     "cannot parse vector because item at position "
                     "%zu of vector was malformed!", i);
            }
        }

        return_mallocd_expr(.type = SHSL_EXPR_VEC,
                            .vec_expr = (shsl_vec_expr) {
                                .elts = elt_exprs,
                                .size = shsl_vec_length(form),
                            });
    }

    case SHSL_MAP: {
        shsl_expr** key_exprs = (shsl_expr**)calloc(form.ptr->map.size,
                                                    sizeof(shsl_expr*));
        shsl_expr** val_exprs = (shsl_expr**)calloc(form.ptr->map.size,
                                                    sizeof(shsl_expr*));

        for(size_t i = 0; i<form.ptr->map.size; ++i) {
            shsl_expr* next_key = shsl_form_to_expr(form.ptr->map.buf[i].k, env);
            shsl_expr* next_val = shsl_form_to_expr(form.ptr->map.buf[i].v, env);

            if(!(shsl_expr_is_error(next_key) || shsl_expr_is_error(next_val))) {
                key_exprs[i] = next_key;
                val_exprs[i] = next_val;
            }
            else {
                for(size_t j = 0; j<i; ++j) {
                    shsl_expr_free(key_exprs[j]);
                    shsl_expr_free(val_exprs[j]);
                }
                free(key_exprs);
                free(val_exprs);
                if(shsl_expr_is_error(next_key))
                    return shsl_expr_error
                        (next_key->literal,
                         "cannot parse map literal because key at position "
                         "%zu of literal was malformed!", (size_t)i);
                else
                    return shsl_expr_error
                        (next_val->literal,
                         "cannot parse map literal because value at position "
                         "%zu of literal was malformed!", (size_t)i);
            }
        }

        return_mallocd_expr(.type = SHSL_EXPR_MAP,
                            .map_expr = (shsl_map_expr) {
                                .keys = key_exprs,
                                .vals = val_exprs,
                                .size = form.ptr->map.size,
                            });
    }

    case SHSL_ERR:
        return shsl_expr_error
            (form, "an error object was passed to the parser");

    case SHSL_CONS: {
        if(!shsl_is_proper_list(form))
            return shsl_expr_error
                (form, "not well formed list, cannot parse into expression");

        size_t form_length = (size_t)shsl_list_length(form);

        if(form.ptr->cons.car.ptr->type == SHSL_SYM) {
            char* s = shsl_sym_name(shsl_car(form));
            if(strcmp(s, "quote") == 0) {
                if(form_length != 2)
                    return
                        shsl_expr_error
                        (form,
                         "malformed quote form, length of quote form must be "
                         "exactly 2, just the 'quote symbol and the quoted "
                         "datum");
                shsl_ref literal = shsl_nth(form, 1);
                if(shsl_is_err(literal))
                    return shsl_expr_error
                        (literal,
                         "cannot parse quote expression because"
                         "literal in quote was malformed");
                return_mallocd_expr(.type = SHSL_EXPR_LITERAL,
                                    .literal = shsl_ref_add(literal)); 
            }

            else if(strcmp(s, "if") == 0) {
                if(form_length < 3)
                    return shsl_expr_error
                        (form, "malformed if expression, too short");
                if(form_length > 4)
                    return shsl_expr_error
                        (form, "malformed if expression, too long");
                                
                shsl_expr** body = shsl_form_list_to_expr_arr(shsl_cdr(form),
                                                              env);
                ssize_t  err_ind = shsl_expr_arr_find_err(body, form_length-1);
                if(err_ind != -1) {
                    // get error
                    shsl_expr* err_expr = body[err_ind];
                    // free everything we allocated that was not the error
                    shsl_free_expr_arr(body, err_ind);
                    // return propagated error to caller
                    switch(err_ind) {
                    case 0:
                        return shsl_expr_further_error
                            (err_expr,
                             "cannot parse if expression, malfomed condition!");
                    case 1:
                        return shsl_expr_further_error
                            (err_expr,
                             "cannot parse if expression, malfomed then part!");
                    case 2:
                        return shsl_expr_further_error
                            (err_expr,
                             "cannot parse if expression, malfomed else part!");
                    default:
                        assert(0 && "unreachable");
                    }
                }

                shsl_expr* c = body[0];
                shsl_expr* t = body[1];
                // (if <condition> <then>) becomes (if <condition> <then> nil)
                // gotta handle that case
                shsl_expr* e;
                if(form_length == 4)
                    e = body[2];
                else {
                    e = (shsl_expr*)malloc(sizeof(shsl_expr));
                    *e = (shsl_expr) {
                        .type = SHSL_EXPR_LITERAL,
                        .literal = shsl_ref_to_nil(),
                    };
                }
                free(body);

                return_mallocd_expr(.type = SHSL_EXPR_IF,
                                    .if_expr = (shsl_if_expr) {
                                        .condition = c,
                                        .then_part = t,
                                        .else_part = e
                                    });
            }
            else if(strcmp(s, "let") == 0) {
                if(form_length < 2)
                    return shsl_expr_error
                        (form,
                         "malformed let expression, where are the bindings, "
                         "where is anything, there's nothing here!");

                // (let [ <key val>* ] <body-exprs>*) 
                if(!shsl_is_vec(shsl_nth(form, 1)))
                    return shsl_expr_error
                        (form,
                         "malformed let statement! bindings spec in let "
                         "statement must be a vector!");

                shsl_ref binds_vec = shsl_nth(form, 1);
                size_t binds_length = shsl_vec_length(binds_vec);
                if(binds_length%2 != 0)
                    return shsl_expr_error
                        (form,
                         "malformed let statement! bindings spec vector in let "
                         "statement must have even length, provided vector has "
                         "uneven length %zu!", binds_length);

                for(size_t i = 0; i<shsl_vec_length(binds_vec); i+=2) {
                    if(!shsl_is_sym(shsl_vec_get(binds_vec, i)))
                        return shsl_expr_error
                            (form,
                             "malformed let statement!"
                             "all keys in bindings vector must be symbols!");
                }

                shsl_ref keys = shsl_mkvec(binds_length/2);
                for(size_t i = 0; i<shsl_vec_length(binds_vec); i+=2)
                    shsl_vec_push(keys,shsl_vec_get(binds_vec, i));

                shsl_expr** vals = (shsl_expr**)calloc(binds_length/2,
                                                       sizeof(shsl_expr*)); 
                for(size_t i = 1; i<shsl_vec_length(binds_vec); i+=2) {
                    shsl_expr* next =
                        shsl_form_to_expr(shsl_vec_get(binds_vec, i), env);
                    if(!shsl_expr_is_error(next))
                        vals[i/2] = next;
                    else {
                        for(size_t j = 1; j<i; j+=2)
                            shsl_expr_free(vals[i]);
                        free(vals);
                        return shsl_expr_further_error
                            (next,
                             "cannot parse let expression, malfomed value "
                             "expression in bindings vector");
                    }
                }

                shsl_expr** body = shsl_form_list_to_expr_arr
                    (shsl_nthcdr(form, 2), env);
                ssize_t  err_ind = shsl_expr_arr_find_err(body, form_length-2);
                if(err_ind != -1) {
                    shsl_expr* err_expr = body[err_ind];
                    shsl_free_expr_arr(body, err_ind);
                    shsl_free_expr_arr(vals, binds_length/2);
                    shsl_free(keys);

                    err_ind++; // use 1 based indexing for the error reporting 
                    return shsl_expr_further_error
                        (err_expr,
                         "cannot parse let expression, malformed subexpression "
                         "in body at position %zu", (size_t)err_ind);
                }
                return_mallocd_expr
                    (.type=SHSL_EXPR_LET,
                     .let_expr = (shsl_let_expr) {
                         .keys = shsl_ref_add(keys),
                         .vals = vals,
                         .binds_length = binds_length/2,
                         .body = body,
                         .body_length = form_length - 2,
                     });
            }
            else if(strcmp(s, "while") == 0) {
                if(form_length < 2)
                    return shsl_expr_error
                        (form,
                         "malformed while expression, it doesn't even have a "
                         "loop condition, there's nothing here!");

                // (while <expr> <body-exprs>*)
                shsl_expr* condition = shsl_form_to_expr(shsl_nth(form, 1), env);
                if(shsl_expr_is_error(condition))
                    return shsl_expr_further_error
                        (condition,
                         "cannot parse while expression, malformed loop "
                         "condition!");

                shsl_expr** body = shsl_form_list_to_expr_arr
                    (shsl_nthcdr(form, 2), env);
                ssize_t  err_ind = shsl_expr_arr_find_err(body, form_length-2);
                if(err_ind != -1) {
                    shsl_expr* err_expr = body[err_ind];
                    shsl_expr_free(condition);
                    shsl_free_expr_arr(body, err_ind);
                    err_ind++; // use 1 based indexing for the error reporting 
                    return shsl_expr_further_error
                        (err_expr,
                         "cannot parse while expression, malformed "
                         "subexpression in body at position %zu",
                         (size_t)err_ind);
                }

                return_mallocd_expr(.type = SHSL_EXPR_WHILE,
                                    .while_expr = (shsl_while_expr) {
                                        .condition = condition,
                                        .body = body,
                                        .body_length = form_length - 2,
                                    });
            }
            // do forms and do-poking forms are too similar in syntax
            // and creation for two different arms of this if then else
            // to be justifiable imo, so we're merging the handling and
            // dispatching at the end on "which side were we parsing? good"
            else if((strcmp(s, "do") == 0) || (strcmp(s, "do-poking") == 0)) {
                bool are_we_do = (strcmp(s, "do") == 0);
                shsl_expr** body = shsl_form_list_to_expr_arr
                    (shsl_cdr(form), env);
                ssize_t  err_ind = shsl_expr_arr_find_err(body, form_length-1);
                if(err_ind != -1) {
                    shsl_expr* err_expr = body[err_ind];
                    shsl_free_expr_arr(body, err_ind);
                    err_ind++; // use 1 based indexing for the error reporting 
                    return shsl_expr_further_error
                        (err_expr,
                         "cannot parse %s expression because expression at "
                         "position %zu in body was malformed",
                         s, (size_t)err_ind);
                }
                if(are_we_do)
                    return_mallocd_expr(.type = SHSL_EXPR_DO,
                                        .do_expr = (shsl_do_expr) {
                                            .body = body,
                                            .body_length = (form_length-1),
                                        });
                else
                    return_mallocd_expr(.type = SHSL_EXPR_DO_POKING,
                                        .do_poking_expr = (shsl_do_poking_expr) {
                                            .body = body,
                                            .body_length = (form_length-1),
                                        });
            }
            // same reasoning as above
            else if((strcmp(s, "set") == 0) || (strcmp(s, "def") == 0)) {
                bool are_we_set = (strcmp(s, "set") == 0);
                // TODO
                // we currently only support (set/def <name> <val>)
                // I'd like set to be destructuring
                if(form_length != 3)
                    return shsl_expr_error
                        (form,
                         "%s expression must be of the for (%s <name> <val>) "
                         "and thus have length of exactly 3, "
                         "invalid %s expression length"
                         "of %zu", s, s, s, form_length);
                if(!shsl_is_sym(shsl_nth(form, 1)))
                    return shsl_expr_error
                        (form,
                         "invalid %s expression, in (%s <name> <val>) "
                         "<name> must be a symbol, provided name is not "
                         "a symbol", s, s);
                shsl_expr* value = shsl_form_to_expr(shsl_nth(form, 2), env);
                if(shsl_expr_is_error(value))
                    return shsl_expr_further_error
                        (value,
                         "cannot parse %s expression because value "
                         " is malformed", s);
                if(are_we_set)
                    return_mallocd_expr
                        (.type = SHSL_EXPR_SET,
                         .set_expr = (shsl_set_expr) {
                             .name = shsl_ref_add(shsl_nth(form, 1)),
                             .value = value,
                         });
                else
                    return_mallocd_expr
                        (.type = SHSL_EXPR_DEF,
                         .def_expr = (shsl_def_expr) {
                             .name = shsl_ref_add(shsl_nth(form, 1)),
                             .value = value,
                         });
            }
            // same reasoning as above

            else if((strcmp(s, "fn") == 0) || (strcmp(s, "macro") == 0)) {
                bool are_we_fn = (strcmp(s, "fn") == 0);
                // malformed lambda list should be caught at compiletime so
                // lambda list validation is gonna go here
                // and since I like to have validation and parsing logic together
                // lambda list creation is also gonna go here
                // 
                // also, we (currently) want the memory used for parsed objects
                // to be as disjoint as possible from the memory used for live
                // objects, so when the lambda list we creaeted goes from
                // parsed object to live object... we're likely have to copy
                // a bunch of shit
                // 
                // a mildly more annoying consequence of this isolation we seek
                // is that the parsed function/macro body will also have to be
                // copied when passing it to the live function object

                // (fn/macro [things] body)
                // we don't support multimethods for now :/
                if(form_length < 2)
                    return shsl_expr_error
                        (form,
                         "malformed %s expression, a fn expression must have "
                         "a lambda list and a body, "
                         "this doesn't have a damn thing!", s);

                shsl_ref ll_form = shsl_nth(form, 1);
                if(!shsl_is_vec(ll_form))
                    return shsl_expr_error
                        (form,
                         "malformed %s expression, second element of function "
                         "definition should be a symbol array, "
                         "but here it's not even an array!", s);

                shsl_vec_foreach(i, elt, ll_form) {
                    if(!shsl_is_sym(elt)) {
                        // use 1-indexing for error reporting
                        // altering the index is not a problem since we're
                        // not gona use it after this anyway
                        i++; 
                        return shsl_expr_error
                            (form,
                             "malformed %s expression, all arguments "
                             "must be symbols, but %zu%s argument in the "
                             "lambda list of this definition was not a symbol!",
                             s,
                             (size_t)i,
                             i==1?"st":i==2?"nd":i==3?"rd":"th");
                    }
                }
                // we currently only support positional or variadics with one
                // variadic argument
                // see if we're variadic
                // (we mark the last symbol in the lambda list as refering to a
                //  a variadic argument if the second to last element in the lambda
                //  list is the symbol '&
                //  if the symbol '& appears anywhere but in the second to last
                //  position in the lambda list we consider that to be an error)
                bool are_we_variadic = false;
                shsl_vec_foreach(i, elt, ll_form) {
                    if(strcmp(shsl_sym_name(elt), "&") == 0) {
                        // we found a '&, but is the '& in the right place?
                        if(i == shsl_vec_length(shsl_nth(form, 1)) - 2) {
                            // is it really?
                            if(strcmp(shsl_sym_name(shsl_vec_last(ll_form)), "&")
                               == 0) {
                                return shsl_expr_error
                                    (form,
                                     "&, when it appears in a lambda list, "
                                     "should appear in the second to last "
                                     "position of a lambda list, and in the "
                                     "second to last position ONLY, what the "
                                     "fuck is this supposed to be?");
                            }
                            // ok it is
                            are_we_variadic = true;
                            break;
                        }
                        else {
                            // '& was not in the right place, report this fact
                            // to the authorities
                            // 
                            // use 1-indexing for the error reporting
                            // the (1-indexed) position '& is actually at
                            size_t ac = i+1;
                            // the (1-indexed) position in which '& was expected to
                            // be at instead
                            size_t ex = shsl_vec_length(shsl_nth(form, 1)) - 2 + 1;
                            return shsl_expr_error
                                (form,
                                 "malformed lambda list in %s expression, "
                                 "'&, used to mark last argument as a variadic, "
                                 "must appear in the second to last position in "
                                 "the lambda list, but here "
                                 "it appears in the %zu%s position, whereas the "
                                 "second to last position would be the %zu%s!",
                                 (size_t)ac,
                                 ac==1?"st":ac==2?"nd":ac==3?"rd":"th",
                                 (size_t)ex,
                                 ex==1?"st":ex==2?"nd":ex==3?"rd":"th");
                        }
                    }
                }

                // create lambda list
                shsl_lambda_list* ll
                    = (shsl_lambda_list*)malloc(sizeof(shsl_lambda_list));

                if(!are_we_variadic) {
                    // purely positional lambda list
                    ll->all = shsl_ref_add(ll_form);
                    ll->variadic = shsl_ref_to_nil();
                    ll->keyword = shsl_ref_add(shsl_mkmap(1));
                }
                else {
                    // add the variadic argument
                    ll->all = shsl_ref_add(ll_form);
                    ll->variadic = shsl_ref_add(shsl_vec_last(ll_form));
                    ll->keyword = shsl_ref_add(shsl_mkmap(1));
                }

                shsl_expr** body = shsl_form_list_to_expr_arr
                    (shsl_nthcdr(form, 2), env);
                ssize_t err_ind = shsl_expr_arr_find_err(body, form_length-2);
                if(err_ind != -1) {
                    shsl_expr* err_expr = body[err_ind];
                    shsl_free_expr_arr(body, err_ind);
                    err_ind++; // use 1 based indexing for the error reporting 
                    return shsl_expr_further_error
                        (err_expr,
                         "cannot parse function expressioned, %zu%s "
                         "expression in function body was malformed!",
                         (size_t)err_ind,
                         err_ind==1?"st":err_ind==2?"nd":err_ind==3?"rd":"th");
                }
                if(are_we_fn)
                    return_mallocd_expr(.type = SHSL_EXPR_FN,
                                        .fn_expr = (shsl_fn_expr) {
                                            .lambda_list = ll,
                                            .body = body,
                                            .body_length = form_length-2,
                                        });
                else
                    return_mallocd_expr(.type = SHSL_EXPR_MACRO,
                                        .macro_expr = (shsl_macro_expr) {
                                            .lambda_list = ll,
                                            .body = body,
                                            .body_length = form_length-2,
                                        });
            }
        }

        // if none of the above matched then it's a funcall
        // but we got two options now, is this a function call or a macro call?

        // macros should be discoverable at compile time so we're gonna do a
        // *bit* of compile time evaluation to figure out wether expr is a macro
        // invocation
        // 
        // first of all, to make sure we're not fucking anything up by doing
        // compile time evaluation, we'll say macros can only result
        // from inline macros (if fn_expr is a macro expr)
        // or as the result of a lookup (if fn_expr is a lookup expr)
        // (with inline macros and this shit being all at runtime you could
        //  probably compute macros at runtime and do some weird shit like that
        //  but we're gonna just... not consider that case to exist for now)
        shsl_expr* fn_expr = shsl_form_to_expr(form.ptr->cons.car, env);
        shsl_ref fn = shsl_ref_to_nil();
        if(fn_expr->type == SHSL_EXPR_MACRO) {
            fn = shsl_eval(fn_expr, env);
        }
        else if(fn_expr->type == SHSL_EXPR_LOOKUP) {
            // to avoid generating potential runtime errors at compiletime
            // whenever a lookup for for a potential macro fails during
            // macroexpansion time we're gonan use shsl_env_lookup_or
            // instead of the regular shsl_env_lookup
            // that would have been invoked by shsl_eval
            // 
            // this will return a default value on lookup failure
            // instead of returning (and potentially logging) an error
            // and as long as the default return value is not a macro, doesn't
            // change shit for our purposes
            fn = shsl_env_lookup_or(env,
                                    fn_expr->lookup_symbol,
                                    shsl_ref_to_nil());
        }

        if(shsl_is_macro(fn)) {
            // macros are functions and functions expect their arguments inside
            // a vector
            shsl_cb args_builder = shsl_cb_make(SHSL_CB_VEC);
            for(shsl_ref i = shsl_cdr(form); shsl_is_cons(i); i = shsl_cdr(i)) {
                shsl_cb_add(&args_builder, shsl_car(i));
            }
            shsl_ref args_vec = shsl_cb_get(args_builder);
            switch(shsl_type(fn)) {
            case SHSL_BUILTIN_MACRO: {
                shsl_ref new_form = shsl_call_builtin_fn(fn, args_vec);
                shsl_ref_add(new_form);
                shsl_expr* new_expr = shsl_form_to_expr(new_form, env);
                shsl_ref_drop(new_form);
                return new_expr;
            }
            case SHSL_USER_MACRO: {
                shsl_ref new_form = shsl_call_user_fn(fn, args_vec);
                shsl_ref_add(new_form);
                shsl_expr* new_expr = shsl_form_to_expr(new_form, env);
                shsl_ref_drop(new_form);
                return new_expr;
            }
            default:
                assert(0 && "this was supposed to be unreachable, I don't know "
                       "how you got here but that's your problem now");
            }
        }
        else {
            shsl_expr** args = shsl_form_list_to_expr_arr(shsl_cdr(form), env);
            ssize_t  err_ind = shsl_expr_arr_find_err(args, form_length-1);
            if(err_ind != -1) {
                shsl_expr* err_arg = args[err_ind];
                shsl_free_expr_arr(args, err_ind);
                err_ind++; // use 1 based indexing for the error reporting 
                return shsl_expr_further_error
                    (err_arg,
                     "cannot parse function call because functions's "
                     "%zu%s argument was a malformed expression",
                     (size_t)err_ind,
                     err_ind==1?"st":err_ind==2?"nd":err_ind==3?"rd":"th");
                // this printing logic doesn't handle things like 32nd
                // but if you pass 32 args to a function that's a you problem
            }
            else
                return_mallocd_expr(.type = SHSL_EXPR_FUNCALL,
                                    .funcall_expr = (shsl_funcall_expr) {
                                        .fn_expr = fn_expr,
                                        .args = args,
                                        .args_length = form_length - 1,
                                    });
        }
    }
        break;
    case SHSL_BUILTIN_FN:
    case SHSL_USER_FN:
    case SHSL_BUILTIN_MACRO:
    case SHSL_USER_MACRO:
        return shsl_expr_error
            (form,
             "cannot include function object in source code of expression!"); 
    }
    assert(0 && "unreachable");
}
shsl_ref shsl_macroexpand_1(shsl_ref form, shsl_ref env) {
    // this is the same logic of the macro expansion we do in shsl_form_to_expr
    // (it's more or less copied from there)
    // but this function exists mainly for debugging purposes
    // it is *not* used for macro expansion during shsl's execution pipeline
    if(shsl_type(form) != SHSL_CONS)
        return form;

    shsl_expr* fn_expr = shsl_form_to_expr(form.ptr->cons.car, env);
    shsl_ref fn = shsl_ref_to_nil();
    if(fn_expr->type == SHSL_EXPR_MACRO)
        fn = shsl_eval(fn_expr, env);
    if(fn_expr->type == SHSL_EXPR_LOOKUP)
        fn = shsl_env_lookup_or(env,
                                fn_expr->lookup_symbol,
                                shsl_ref_to_nil());

    if(shsl_is_macro(fn)) {
        shsl_cb args_builder = shsl_cb_make(SHSL_CB_VEC);
        for(shsl_ref i = shsl_cdr(form); shsl_is_cons(i); i = shsl_cdr(i)) {
            shsl_cb_add(&args_builder, shsl_car(i));
        }
        shsl_ref args_vec = shsl_cb_get(args_builder);

        switch(shsl_type(fn)) {
        case SHSL_BUILTIN_MACRO:
            return shsl_call_builtin_fn(fn, args_vec);
        case SHSL_USER_MACRO:
            return shsl_call_user_fn(fn, args_vec);
        default:
            assert(0 && "unreachable");
        }
    }
    else return form;
}

void shsl_expr_free(shsl_expr* expr) {
    // recall
    // expressions don't own any shsl_obj they might contain
    // they only hold a reference to it
    // only delete references to objects they hold, don't free them outright
    // expressions *do* own their subexpressions
    // free any subexpression of the expression you're freeing rn
    switch(expr->type) {
    case SHSL_EXPR_LITERAL:
        shsl_ref_drop(expr->literal);
        free(expr);
        break;
    case SHSL_EXPR_LOOKUP:
        shsl_ref_drop(expr->lookup_symbol);
        free(expr);
        break;
    case SHSL_EXPR_VEC:
        shsl_free_expr_arr(expr->vec_expr.elts, expr->vec_expr.size);
        free(expr);
        break;
    case SHSL_EXPR_MAP:
        shsl_free_expr_arr(expr->map_expr.keys, expr->map_expr.size);
        shsl_free_expr_arr(expr->map_expr.vals, expr->map_expr.size);
        free(expr);
        break;
    case SHSL_EXPR_IF:
        shsl_expr_free(expr->if_expr.condition);
        shsl_expr_free(expr->if_expr.then_part);
        shsl_expr_free(expr->if_expr.else_part);
        free(expr);
        break;
    case SHSL_EXPR_LET:
        shsl_ref_drop(expr->let_expr.keys);
        shsl_free_expr_arr(expr->let_expr.vals, expr->let_expr.binds_length);
        shsl_free_expr_arr(expr->let_expr.body, expr->let_expr.body_length);
        free(expr);
        break;
    case SHSL_EXPR_WHILE:
        shsl_expr_free(expr->while_expr.condition);
        shsl_free_expr_arr(expr->while_expr.body, expr->while_expr.body_length);
        free(expr);
        break;
    case SHSL_EXPR_DEF:
        shsl_ref_drop(expr->def_expr.name);
        shsl_expr_free(expr->def_expr.value);
        free(expr);
        break;
    case SHSL_EXPR_SET:
        shsl_ref_drop(expr->set_expr.name);
        shsl_expr_free(expr->set_expr.value);
        free(expr);
        break;
    case SHSL_EXPR_FN:
        shsl_ref_drop(expr->fn_expr.lambda_list->all);
        shsl_ref_drop(expr->fn_expr.lambda_list->variadic);
        shsl_ref_drop(expr->fn_expr.lambda_list->keyword);
        free(expr->fn_expr.lambda_list);
        shsl_free_expr_arr(expr->fn_expr.body, expr->fn_expr.body_length);
        free(expr);
        break;
    case SHSL_EXPR_MACRO:
        shsl_ref_drop(expr->macro_expr.lambda_list->all);
        shsl_ref_drop(expr->macro_expr.lambda_list->variadic);
        shsl_ref_drop(expr->macro_expr.lambda_list->keyword);
        free(expr->macro_expr.lambda_list);
        shsl_free_expr_arr(expr->macro_expr.body, expr->macro_expr.body_length);
        free(expr);
        break;

    case SHSL_EXPR_DO:
        shsl_free_expr_arr(expr->do_expr.body, expr->do_expr.body_length);
        free(expr);
        break;
    case SHSL_EXPR_DO_POKING:
        shsl_free_expr_arr(expr->do_poking_expr.body,
                           expr->do_poking_expr.body_length);
        free(expr);
        break;
    case SHSL_EXPR_FUNCALL:
        shsl_expr_free(expr->funcall_expr.fn_expr);
        shsl_free_expr_arr(expr->funcall_expr.args,
                           expr->funcall_expr.args_length);
        free(expr);
        break;
    }
}
// we need this function because I want function literals to return a fresh
// function that won't be affected by deleting the expression that originated it
// which means function literals must copy their body into that of the function
// they return
shsl_expr* shsl_expr_copy(shsl_expr* orig) {
    switch(orig->type) {
    case SHSL_EXPR_LITERAL:
        return_mallocd_expr
            (.type = SHSL_EXPR_LITERAL,
             .literal = shsl_ref_add(shsl_copy(orig->literal)));
    case SHSL_EXPR_LOOKUP:
        return_mallocd_expr
            (.type = SHSL_EXPR_LOOKUP,
             .lookup_symbol = shsl_ref_add(shsl_copy(orig->literal)));
    case SHSL_EXPR_VEC: {
        size_t size = orig->vec_expr.size;
        shsl_expr** elts = (shsl_expr**)calloc(size, sizeof(shsl_expr*));
        for(size_t i = 0; i<size; ++i)
            elts[i] = shsl_expr_copy(orig->vec_expr.elts[i]);

        return_mallocd_expr(.type = SHSL_EXPR_VEC,
                            .vec_expr = (shsl_vec_expr) {
                                .elts = elts,
                                .size = size,
                            });
    }
    case SHSL_EXPR_MAP: {
        size_t size = orig->map_expr.size;
        shsl_expr** keys = (shsl_expr**)calloc(size, sizeof(shsl_expr*));
        shsl_expr** vals = (shsl_expr**)calloc(size, sizeof(shsl_expr*));
        for(size_t i = 0; i<size; ++i) {
            keys[i] = shsl_expr_copy(orig->map_expr.keys[i]);
            vals[i] = shsl_expr_copy(orig->map_expr.vals[i]);
        }
        return_mallocd_expr(.type = SHSL_EXPR_VEC,
                            .map_expr = (shsl_map_expr) {
                                .keys = keys,
                                .vals = vals,
                                .size = size,
                            });
    }
    case SHSL_EXPR_IF:
        return_mallocd_expr
            (.type = SHSL_EXPR_IF,
             .if_expr = (shsl_if_expr) {
                 .condition = shsl_expr_copy(orig->if_expr.condition),
                 .then_part = shsl_expr_copy(orig->if_expr.then_part),
                 .else_part = shsl_expr_copy(orig->if_expr.else_part),
             });
    case SHSL_EXPR_DO: {
        size_t body_length = orig->do_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(orig->do_expr.body[i]);

        return_mallocd_expr
            (.type = SHSL_EXPR_DO,
             .do_expr = (shsl_do_expr) {
                 .body = body,
                 .body_length = body_length,
             });
    }
    case SHSL_EXPR_DO_POKING: {
        size_t body_length = orig->do_poking_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(orig->do_poking_expr.body[i]);

        return_mallocd_expr
            (.type = SHSL_EXPR_DO_POKING,
             .do_poking_expr = (shsl_do_poking_expr) {
                 .body = body,
                 .body_length = body_length,
             });
    }
    case SHSL_EXPR_LET: {
        size_t body_length = orig->let_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(orig->let_expr.body[i]);

        size_t binds_length = orig->let_expr.binds_length;
        shsl_expr** vals = (shsl_expr**)calloc(binds_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<binds_length; ++i)
            vals[i] = shsl_expr_copy(orig->let_expr.vals[i]);

        return_mallocd_expr
            (.type = SHSL_EXPR_LET,
             .let_expr = (shsl_let_expr) {
                 .keys = shsl_ref_add(shsl_copy(orig->let_expr.keys)),
                 .vals = vals,
                 .binds_length = binds_length,
                 .body = body,
                 .body_length = body_length,
             });
    }

    case SHSL_EXPR_WHILE: {
        size_t body_length = orig->while_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(orig->while_expr.body[i]);

        return_mallocd_expr
            (.type = SHSL_EXPR_WHILE,
             .while_expr = (shsl_while_expr) {
                 .condition = shsl_expr_copy(orig->while_expr.condition),
                 .body = body,
                 .body_length = body_length,
             });
    }
    case SHSL_EXPR_DEF:
        return_mallocd_expr
            (.type = SHSL_EXPR_DEF,
             .set_expr = (shsl_set_expr) {
                 .name = shsl_ref_add(shsl_copy(orig->def_expr.name)),
                 .value = shsl_expr_copy(orig->def_expr.value),
             });
    case SHSL_EXPR_SET:
        return_mallocd_expr
            (.type = SHSL_EXPR_SET,
             .set_expr = (shsl_set_expr) {
                 .name = shsl_ref_add(shsl_copy(orig->set_expr.name)),
                 .value = shsl_expr_copy(orig->set_expr.value),
             });
    case SHSL_EXPR_FN: {
        size_t body_length = orig->fn_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(orig->fn_expr.body[i]);
        shsl_lambda_list* lambda_list =
            (shsl_lambda_list*)malloc(sizeof(shsl_lambda_list));
        lambda_list->all=
            shsl_ref_add(shsl_copy(orig->fn_expr.lambda_list->all));
        lambda_list->variadic=
            shsl_ref_add(shsl_copy(orig->fn_expr.lambda_list->variadic));
        lambda_list->keyword=
            shsl_ref_add(shsl_copy(orig->fn_expr.lambda_list->keyword));
        return_mallocd_expr
            (.type = SHSL_EXPR_FN,
             .fn_expr = (shsl_fn_expr) {
                 .lambda_list = lambda_list,
                 .body = body,
                 .body_length = body_length,
             });
    }
    case SHSL_EXPR_MACRO: {
        size_t body_length = orig->macro_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(orig->macro_expr.body[i]);
        shsl_lambda_list* lambda_list =
            (shsl_lambda_list*)malloc(sizeof(shsl_lambda_list));
        lambda_list->all=
            shsl_ref_add(shsl_copy(orig->macro_expr.lambda_list->all));
        lambda_list->variadic=
            shsl_ref_add(shsl_copy(orig->macro_expr.lambda_list->variadic));
        lambda_list->keyword=
            shsl_ref_add(shsl_copy(orig->macro_expr.lambda_list->keyword));
        return_mallocd_expr
            (.type = SHSL_EXPR_MACRO,
             .macro_expr = (shsl_macro_expr) {
                 .lambda_list = lambda_list,
                 .body = body,
                 .body_length = body_length,
             });
    }
    case SHSL_EXPR_FUNCALL: {
        size_t args_length = orig->funcall_expr.args_length;
        shsl_expr** args = (shsl_expr**)calloc(args_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<args_length; ++i)
            args[i] = shsl_expr_copy(orig->funcall_expr.args[i]);

        return_mallocd_expr
            (.type = SHSL_EXPR_FUNCALL,
             .funcall_expr = (shsl_funcall_expr) {
                 .fn_expr = shsl_expr_copy(orig->funcall_expr.fn_expr),
                 .args = args,
                 .args_length = args_length,
             });
    }
    }
    assert(0 && "UNREACHABLE");
}

/// EVALUATION FUNCTIONS DEFINITIONS
// this function is to be called by c code that is sure syms and vals have the
// same length, it is guaranteed to return a valid env frame
// well, that or it shits itself and dies because it cannot return one
shsl_ref shsl_env_mkframe(shsl_ref syms, shsl_ref vals) {
    assert(syms.ptr->type == SHSL_VEC);
    assert(vals.ptr->type == SHSL_VEC);
    assert(shsl_vec_length(syms) == shsl_vec_length(vals));

    shsl_ref frame = shsl_mkmap(shsl_vec_length(syms));
    shsl_vec_foreach(i, sym, syms) {
        shsl_ref val = shsl_vec_get(vals, i);
        shsl_map_set(frame, sym, val);
    }
    return frame;
}
// this function is instead called by shsl code during function calls,
// it doesn't crash if it cannot return a valid env frame, because we don't want
// shsl to crash if given incorrect input, we want it to instead report an error
// to the caller, so this function *may* return an error, or it may return a
// valid env frame.
// callers of this function should make sure the value they get is a valid env 
// frame, or they're gonna get some weird fucking bugs when looking for symbols
// in error objects
shsl_ref shsl_ll_env_mkframe(shsl_lambda_list* ll, shsl_ref vals) {
    shsl_ref syms = ll->all;
    if(shsl_is_nil(ll->variadic)) {
        // purely positional
        if(shsl_vec_length(syms) != shsl_vec_length(vals))
            return shsl_mkerr
                (vals,
                 "expected exactly %zu positional arguments but %zu were provided",
                 shsl_vec_length(syms), shsl_vec_length(vals));
        return shsl_env_mkframe(syms, vals);
    }
    else {
        // with variadics
        // sanity check
        // (if this assert fails it's a c side error and we should therefore crash)
        assert(shsl_is_sym(ll->variadic));

        size_t n_positional = shsl_vec_length(syms)-2;
        size_t n_variadic = shsl_vec_length(vals)-n_positional;
        if(n_positional > shsl_vec_length(vals))
            return shsl_mkerr
                (vals,
                 "expected at least %zu arguments but %zu arguments were provided",
                 n_positional, shsl_vec_length(vals));

        // positional syms and positional vals
        shsl_ref psyms = shsl_ref_add(shsl_vec_slice(syms, 0, n_positional));
        shsl_ref pvals = shsl_ref_add(shsl_vec_slice(vals, 0, n_positional));
        // variadic sym and variadic vals (vector)
        shsl_ref vsym = ll->variadic;
        shsl_ref vvals = shsl_vec_slice(vals, n_positional,
                                        n_positional + n_variadic);

        shsl_ref frame = shsl_env_mkframe(psyms, pvals);
        shsl_ref_drop(psyms);
        shsl_ref_drop(pvals);
        shsl_map_set(frame, vsym, vvals);
        return frame;
    }
}

shsl_kv* shsl_env_find_kv(shsl_ref env, shsl_ref key) {
    assert(shsl_type(key) == SHSL_SYM);
    if(shsl_is_nil(env))
        return NULL;

    assert(shsl_type(env) == SHSL_CONS);
    assert(shsl_type(shsl_car(env)) == SHSL_MAP);

    ssize_t i = shsl_map_index(shsl_car(env), key);
    if(i>=0)
        return &(env.ptr->cons.car.ptr->map.buf[i]);
    return shsl_env_find_kv(shsl_cdr(env), key);
}
shsl_ref shsl_env_lookup(shsl_ref env, shsl_ref key) {
    shsl_kv* kv = shsl_env_find_kv(env, key);
    if(!kv)
        return shsl_mkerr(key, "symbol not found in environment!");
    return kv->v;
}
shsl_ref shsl_env_lookup_or(shsl_ref env, shsl_ref key, shsl_ref def) {
    shsl_kv* kv = shsl_env_find_kv(env, key);
    if(!kv)
        return def;
    return kv->v;
}
bool shsl_env_has(shsl_ref env, shsl_ref key) {
    shsl_kv* kv = shsl_env_find_kv(env, key);
    return kv != NULL;
}

shsl_ref shsl_env_set(shsl_ref env, shsl_ref key, shsl_ref new_val) {
    assert(shsl_is_sym(key));
    assert(shsl_is_cons(env));
    assert(shsl_is_map(shsl_car(env)));

    shsl_kv* kv = shsl_env_find_kv(env, key);
    if(!kv)
        shsl_map_set(shsl_car(env), key, new_val);
    else
        shsl_ref_set(&(kv->v), new_val);
    // TODO:
    // shitty incomplete cycle detection for avoiding circular references
    if(shsl_is_fn(new_val)
       && shsl_fn_env(new_val).ptr == env.ptr
       && env.ptr->ref_count > 1)
        shsl_fn_env_mark_weak(new_val);

    return new_val;
}
shsl_ref shsl_env_def(shsl_ref env, shsl_ref key, shsl_ref new_val) {
    assert(shsl_is_sym(key));
    assert(shsl_is_cons(env));
    assert(shsl_is_map(shsl_car(env)));

    shsl_map_set(shsl_car(env), key, new_val);

    if(shsl_is_fn(new_val)
       && shsl_fn_env(new_val).ptr == env.ptr
       && env.ptr->ref_count > 1)
        shsl_fn_env_mark_weak(new_val);

    return new_val;
}

shsl_ref shsl_call_builtin_fn(shsl_ref fn, shsl_ref args) {
    assert(shsl_type(fn) == SHSL_BUILTIN_FN ||
           shsl_type(fn) == SHSL_BUILTIN_MACRO);
    assert(shsl_type(args) == SHSL_VEC);

    shsl_builtin_fn inner_fn = (shsl_type(fn) == SHSL_BUILTIN_FN)
        ?fn.ptr->builtin_fn
        :fn.ptr->builtin_macro;

    return inner_fn.apply (args, inner_fn.env);
}

shsl_ref shsl_call_user_fn(shsl_ref fn, shsl_ref args) {
    assert(shsl_type(fn) == SHSL_USER_FN ||
           shsl_type(fn) == SHSL_USER_MACRO);
    assert(shsl_type(args) == SHSL_VEC);

    shsl_lambda_list* ll = (shsl_type(fn) == SHSL_USER_FN)
        ?fn.ptr->user_fn.lambda_list
        :fn.ptr->user_macro.lambda_list;

    size_t len = fn.ptr->user_fn.body_length;
    if(len == 0)
        return shsl_ref_to_nil();

    shsl_ref inner_frame = shsl_ll_env_mkframe(ll, args);
    if(shsl_is_err(inner_frame)) {
        return shsl_mkerr
            (inner_frame,
             "could not correctly bind arguments during function call!");
    }

    shsl_ref inner_env =
        shsl_mkcons (inner_frame, fn.ptr->user_fn.env);

    shsl_ref_add(inner_env);
    shsl_ref res = shsl_eval_sequence(fn.ptr->user_fn.body,
                                      fn.ptr->user_fn.body_length,
                                      inner_env);

    // shitty hack because I fucked something up in some refcounting
    // data contracts
    res.ptr->ref_count++;
    shsl_ref_drop(inner_env);
    res.ptr->ref_count--;
    return res;
}

shsl_ref shsl_eval_sequence(shsl_expr** seq, size_t seq_length, shsl_ref env) {
    if(seq_length == 0)
        return shsl_ref_to_nil();

    for(size_t i = 0; i<seq_length-1; ++i)
        shsl_ref_free_if_unreachable(shsl_eval(seq[i], env));
    shsl_ref res = shsl_eval(seq[seq_length-1], env);
    return res;
}
shsl_ref shsl_eval(shsl_expr* expr, shsl_ref env) {
    assert(shsl_is_nil(env)
           || (shsl_is_cons(env) && shsl_is_map(env.ptr->cons.car))
           && "if evaluation env is not null it must be a list of maps!");

    switch(expr->type) {
    case SHSL_EXPR_LITERAL:
        return shsl_copy(expr->literal);
    case SHSL_EXPR_LOOKUP:
        return shsl_env_lookup(env, expr->lookup_symbol);
    case SHSL_EXPR_VEC: {
        size_t size = expr->vec_expr.size;
        shsl_ref vec = shsl_mkvec(size);
        for(size_t i = 0; i<size; ++i)
            shsl_vec_push(vec, shsl_eval(expr->vec_expr.elts[i], env));
        return vec;
    }
    case SHSL_EXPR_MAP: {
        size_t size = expr->map_expr.size;
        shsl_ref map = shsl_mkmap(size);
        for(size_t i = 0; i<size; ++i)
            shsl_map_set(map,
                         shsl_eval(expr->map_expr.keys[i], env),
                         shsl_eval(expr->map_expr.vals[i], env));
        return map;
    }
    case SHSL_EXPR_IF: {
        shsl_ref c = shsl_eval(expr->if_expr.condition, env);
        shsl_ref res;
        if(shsl_is_truthy(c))
            res=shsl_eval(expr->if_expr.then_part, env);
        else
            res=shsl_eval(expr->if_expr.else_part, env);
        shsl_ref_free_if_unreachable(c);
        return res;
    }
    case SHSL_EXPR_DO: {
        shsl_ref inner_env = shsl_ref_add(shsl_mkcons(shsl_mkmap(1), env));
        shsl_ref res = shsl_eval_sequence(expr->do_expr.body,
                                          expr->do_expr.body_length,
                                          inner_env);
        res.ptr->ref_count++;
        shsl_ref_drop(inner_env);
        res.ptr->ref_count--;
        return res;
    }
    case SHSL_EXPR_DO_POKING: {
        return shsl_eval_sequence(expr->do_poking_expr.body,
                                  expr->do_poking_expr.body_length,
                                  env);
    }

    case SHSL_EXPR_FUNCALL: {
        shsl_ref fn = shsl_ref_add(shsl_eval(expr->funcall_expr.fn_expr, env));
        shsl_ref args = shsl_ref_add(shsl_eval_many_into_vec
                                     (expr->funcall_expr.args,
                                      expr->funcall_expr.args_length,
                                      env));
        switch(fn.ptr->type) {
        case SHSL_BUILTIN_FN: {
            shsl_ref res = shsl_call_builtin_fn(fn, args);

            // ugly hack to make sure that if res was referenced by args
            // then freeing args doesn't void res
            res.ptr->ref_count++;
            shsl_ref_drop(fn);
            shsl_ref_drop(args);
            res.ptr->ref_count--;
            return res;
        }
        case SHSL_USER_FN: {
            shsl_ref res = shsl_call_user_fn(fn, args);

            res.ptr->ref_count++;
            shsl_ref_drop(fn);
            shsl_ref_drop(args);
            res.ptr->ref_count--;
            return res;
        }
        case SHSL_BUILTIN_MACRO:
            shsl_free(args);
            return shsl_mkerr
                (fn, "macro call should have been handled at macro expand time, "
                 "it is an error to call a macro at eval time"); 
        case SHSL_USER_MACRO:
            shsl_free(args);
            return shsl_mkerr
                (fn, "macro call should have been handled at macro expand time, "
                 "it is an error to call a macro at eval time"); 
            // TODO: case SHSL_SYM:
            // if argument is a map and symbol is a keyword then do the keyword
            // thing
            // further todo: make keywords self evaluating
        default:
            shsl_free(args);
            return shsl_mkerr(fn, "object is not callable!"); 
        }
    }
    case SHSL_EXPR_FN: {
        size_t body_length = expr->fn_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(expr->fn_expr.body[i]);

        // TODO: copy?
        // all our previous eval functions return "all fresh data"
        // this would be a first n which we return data that was partially
        // in an expression
        shsl_lambda_list* lambda_list =
            (shsl_lambda_list*)malloc(sizeof(shsl_lambda_list));
        lambda_list->all = shsl_ref_add
            (expr->fn_expr.lambda_list->all);
        lambda_list->variadic = shsl_ref_add
            (expr->fn_expr.lambda_list->variadic);
        lambda_list->keyword = shsl_ref_add
            (expr->fn_expr.lambda_list->keyword);

        return shsl_mkuser_fn(env, lambda_list, body, body_length);
    }
    case SHSL_EXPR_MACRO: {
        size_t body_length = expr->macro_expr.body_length;
        shsl_expr** body = (shsl_expr**)calloc(body_length, sizeof(shsl_expr*));
        for(size_t i = 0; i<body_length; ++i)
            body[i] = shsl_expr_copy(expr->macro_expr.body[i]);

        // TODO: copy?
        // all our previous eval functions return "all fresh data"
        // this would be a first n which we return data that was partially
        // in an expression
        shsl_lambda_list* lambda_list =
            (shsl_lambda_list*)malloc(sizeof(shsl_lambda_list));
        lambda_list->all = shsl_ref_add
            (expr->macro_expr.lambda_list->all);
        lambda_list->variadic = shsl_ref_add
            (expr->macro_expr.lambda_list->variadic);
        lambda_list->keyword = shsl_ref_add
            (expr->macro_expr.lambda_list->keyword);

        return shsl_mkuser_macro(env, lambda_list, body, body_length);
    }

    case SHSL_EXPR_DEF:
        return shsl_env_def(env,
                            expr->def_expr.name,
                            shsl_eval(expr->def_expr.value, env));
    case SHSL_EXPR_SET:
        return shsl_env_set(env,
                            expr->def_expr.name,
                            shsl_eval(expr->def_expr.value, env));

    case SHSL_EXPR_WHILE: {
        shsl_ref loop_condition = shsl_eval(expr->while_expr.condition, env);
        shsl_ref res = shsl_ref_to_nil();
        while(shsl_is_truthy(loop_condition)) {
            shsl_ref_free_if_unreachable(res);
            res = shsl_eval_sequence(expr->while_expr.body,
                                     expr->while_expr.body_length,
                                     env);
            shsl_ref_free_if_unreachable(loop_condition);
            loop_condition = shsl_eval(expr->while_expr.condition, env);
        }
        return res;
    }
    case SHSL_EXPR_LET: {
        shsl_ref vals = shsl_eval_many_into_vec
            (expr->let_expr.vals,
             expr->let_expr.binds_length,
             env);
        shsl_ref inner_env =
            shsl_mkcons(shsl_env_mkframe(expr->let_expr.keys, vals), env);

        shsl_ref_add(vals);
        shsl_ref_add(inner_env);
        shsl_ref res = shsl_eval_sequence(expr->let_expr.body,
                                          expr->let_expr.body_length,
                                          inner_env);
        res.ptr->ref_count++;
        shsl_ref_drop(vals);
        shsl_ref_drop(inner_env);
        res.ptr->ref_count--;
        return res;
    }
    }
    assert(0 && "unreachable");
}
shsl_ref shsl_eval_many_into_vec(shsl_expr** args, size_t args_length,
                                 shsl_ref env) {
    shsl_ref vec_obj = shsl_mkvec(args_length);
    for(size_t i = 0; i<args_length; ++i) {
        shsl_vec_push(vec_obj, shsl_eval(args[i], env));
    }
    return vec_obj;
}

/// BUILTIN FUNCTIONS DEFINITIONS
// macros to aid in defining a builtin function to call from shsl code
// nothing fancy, just factoring out the boring work
// I might have committed "make a dsl with the c preprocessor" :(, sory
#define shsl_defun(c_name, shsl_name, args_name, env_name, ...)         \
    shsl_ref c_name(shsl_ref args_name, shsl_ref env_name)              \
    {                                                                   \
        const char* shsl_fn_name = shsl_name;   /* used by macros */    \
        shsl_ref shsl_fn_args = args_name; /* same as above */          \
        shsl_fn_assert_vec(shsl_fn_args);                               \
        do __VA_ARGS__ while(0);                                        \
    }                                                                   
#define shsl_fn_arg(i) shsl_vec_get(shsl_fn_args, i)
#define shsl_fn_argslen() shsl_vec_length(shsl_fn_args)
#define shsl_fn_assert(ass) do {                                \
        if(!(ass))                                              \
            return shsl_mkerr(shsl_ref_to_nil(),                \
                              "in function %s:\nAssertion ["    \
                              #ass "] failed!", shsl_fn_name);  \
    } while(0)

#define shsl_fn_assert_vec(args)                \
    shsl_fn_assert(shsl_is_vec(args))
#define shsl_fn_assert_argslen(pred)                            \
    shsl_fn_assert((shsl_vec_length(shsl_fn_args)) pred)
#define shsl_fn_assert_argtype(i, t)                                    \
    shsl_fn_assert(shsl_type(shsl_vec_get(shsl_fn_args, i)) == t)
#define shsl_fn_assert_argtype_either(i, t1, t2)                        \
    shsl_fn_assert(shsl_type(shsl_vec_get(shsl_fn_args, i)) == t1 ||    \
                   shsl_type(shsl_vec_get(shsl_fn_args, i)) == t2)

/// SHSL BUILTIN ARITHMETIC FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_add, "+", args, env, {
        (void)env;
        shsl_vec_foreach(i, elt, args)
            shsl_fn_assert_argtype_either(i, SHSL_INT, SHSL_REAL);

        long intsum = 0;
        double realsum = 0.0;
        shsl_vec_foreach(i, elt, args) {
            if(!shsl_is_int(elt))
                goto realsum;
            else
                intsum += shsl_int(elt);
        }
        return shsl_mkint(intsum);
    realsum:
        shsl_vec_foreach(i, elt, args)
            realsum += shsl_real(elt);
        return shsl_mkreal(realsum);
    })
shsl_defun(shsl_builtin_sub, "-", args, env, {
        (void)env;
        shsl_vec_foreach(i, elt, args)
            shsl_fn_assert_argtype_either(i, SHSL_INT, SHSL_REAL);

        switch(shsl_fn_argslen()) {
        case 0:
            return shsl_mkint(0);
        case 1:
            return shsl_copy(shsl_fn_arg(0));
        default: {
            long intdiff = 0;
            double realdiff;
            if(!shsl_is_int(shsl_fn_arg(0)))
                goto realdiff;
            intdiff = shsl_int(shsl_fn_arg(0));
            for(size_t i = 1; i<shsl_fn_argslen(); ++i)
                if(!shsl_is_int(shsl_fn_arg(i)))
                    goto realdiff;
                else
                    intdiff -= shsl_int(shsl_fn_arg(i));
            return shsl_mkint(intdiff);

            realdiff:
            realdiff = shsl_real(shsl_fn_arg(0));
            for(size_t i = 1; i<shsl_fn_argslen(); ++i)
                realdiff -= shsl_real(shsl_fn_arg(i)); 
            return shsl_mkreal(realdiff);
        }
        }
    })
shsl_defun(shsl_builtin_mul, "*", args, env, {
        (void)env;
        shsl_vec_foreach(i, elt, args)
            shsl_fn_assert_argtype_either(i, SHSL_INT, SHSL_REAL);

        long intprod = 1;
        double realprod = 1.0;
        shsl_vec_foreach(i, elt, args) {
            if(!shsl_is_int(elt))
                goto realprod;
            else
                intprod *= shsl_int(elt);
        }
        return shsl_mkint(intprod);
    realprod:
        shsl_vec_foreach(i, elt, args)
            realprod *= shsl_real(elt);
        return shsl_mkreal(realprod);
    })
shsl_defun(shsl_builtin_div, "/", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype_either(0, SHSL_INT, SHSL_REAL);
        shsl_fn_assert_argtype_either(1, SHSL_INT, SHSL_REAL);

        // handle case where it returns integer
        if(shsl_is_int(shsl_fn_arg(0))
           && shsl_is_int(shsl_fn_arg(1))
           && shsl_int(shsl_fn_arg(0)) != 0
           && ((shsl_int(shsl_fn_arg(1))
                % shsl_int(shsl_fn_arg(0))) == 0))
            return shsl_mkint(shsl_int(shsl_fn_arg(0))
                              / shsl_int(shsl_fn_arg(1)));

        double a = shsl_real(shsl_fn_arg(0));
        double b = shsl_real(shsl_fn_arg(1));
        if(b == 0.0)
            return shsl_mkerr(args, "in function /: division by zero!");
        return shsl_mkreal(a/b);
    })

#define shsl_bool_to_obj(...) ((__VA_ARGS__)?shsl_mksym("t"):shsl_ref_to_nil())
shsl_defun(shsl_builtin_gt, ">", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype_either(0, SHSL_INT, SHSL_REAL);
        shsl_fn_assert_argtype_either(1, SHSL_INT, SHSL_REAL);

        if(shsl_is_int(shsl_fn_arg(0)) && shsl_real(shsl_fn_arg(1)))
            return shsl_bool_to_obj
                (shsl_int(shsl_fn_arg(0)) > shsl_int(shsl_fn_arg(1)));
        return shsl_bool_to_obj
            (shsl_real(shsl_fn_arg(0)) > shsl_real(shsl_fn_arg(1)));
    })
shsl_defun(shsl_builtin_lt, "<", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype_either(0, SHSL_INT, SHSL_REAL);
        shsl_fn_assert_argtype_either(1, SHSL_INT, SHSL_REAL);

        if(shsl_is_int(shsl_fn_arg(0)) && shsl_real(shsl_fn_arg(1)))
            return shsl_bool_to_obj
                (shsl_int(shsl_fn_arg(0)) < shsl_int(shsl_fn_arg(1)));
        return shsl_bool_to_obj
            (shsl_real(shsl_fn_arg(0)) < shsl_real(shsl_fn_arg(1)));
    })
shsl_defun(shsl_builtin_ge, ">=", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype_either(0, SHSL_INT, SHSL_REAL);
        shsl_fn_assert_argtype_either(1, SHSL_INT, SHSL_REAL);

        if(shsl_is_int(shsl_fn_arg(0)) && shsl_real(shsl_fn_arg(1)))
            return shsl_bool_to_obj
                (shsl_int(shsl_fn_arg(0)) >= shsl_int(shsl_fn_arg(1)));
        return shsl_bool_to_obj
            (shsl_real(shsl_fn_arg(0)) >= shsl_real(shsl_fn_arg(1)));
    })
shsl_defun(shsl_builtin_le, "<=", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype_either(0, SHSL_INT, SHSL_REAL);
        shsl_fn_assert_argtype_either(1, SHSL_INT, SHSL_REAL);

        if(shsl_is_int(shsl_fn_arg(0)) && shsl_real(shsl_fn_arg(1)))
            return shsl_bool_to_obj
                (shsl_int(shsl_fn_arg(0)) <= shsl_int(shsl_fn_arg(1)));
        return shsl_bool_to_obj
            (shsl_real(shsl_fn_arg(0)) <= shsl_real(shsl_fn_arg(1)));
    })

/// SHSL BUILTIN DATA FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_isnil, "nil?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_nil(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_isint, "int?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_int(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_isreal, "real?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_real(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_isnum, "num?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_num(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_issym, "sym?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_sym(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_isstr, "str?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_str(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_iserr, "err?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_err(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_iscons, "cons?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_cons(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_isvec, "vec?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_vec(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_ismap, "map?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_map(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_islist, "list?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_list(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_isfn, "fn?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_fn(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_ismacro, "macro?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_macro(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_not, "not", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_bool_to_obj(shsl_is_falsey(shsl_fn_arg(0)));
    })

shsl_defun(shsl_builtin_err, "err", args, env, {
        (void)env;
        shsl_fn_assert_argslen(>= 1);
        shsl_fn_assert_argtype(0, SHSL_STR);

        // shsl's error function takes message first and data later
        // the c version does the opposite out of kinda necessity
        // I wanted to support formatted errors so it made more sense
        // to have the (format) string at the end with the formatted shit
        if(shsl_fn_argslen() == 2)
            return shsl_mkerr(shsl_fn_arg(1), shsl_fn_arg(0).ptr->str);

        shsl_cb rest_builder = shsl_cb_make(SHSL_CB_LIST);
        for(size_t i = 1; i<shsl_fn_argslen(); ++i)
            shsl_cb_add(&rest_builder, shsl_fn_arg(i));

        return shsl_mkerr(shsl_cb_get(rest_builder), shsl_fn_arg(0).ptr->str);
    })

// some more common-lisp-y data functions (I just kinda miss car and cdr)
shsl_defun(shsl_builtin_cons, "cons", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        return shsl_mkcons(shsl_fn_arg(0), shsl_fn_arg(1));
    })

shsl_defun(shsl_builtin_car, "car", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_car(shsl_fn_arg(0));
    })
shsl_defun(shsl_builtin_cdr, "cdr", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        return shsl_cdr(shsl_fn_arg(0));
    })

// nth also works on vectors as it does in clojure
// nthcdr does not work on vectors tho, that wouldn't make much sensE
shsl_defun(shsl_builtin_nth, "nth", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype(1, SHSL_INT);

        int i = shsl_int(shsl_fn_arg(1));
        if(i<0)
            return shsl_mkerr
                (shsl_mkint(i),
                 "nth: tried indexing into collection with negative index "
                 "value %d", i);

        switch(shsl_type(shsl_fn_arg(0))) {
        case SHSL_NIL:
            return shsl_nth(shsl_fn_arg(0), i);
        case SHSL_VEC:
            return shsl_vec_get(shsl_fn_arg(0), i);
        default:
            return shsl_mkerr(shsl_ref_to_nil(),
                              "nth: unrecognized collection type, "
                              "can only operate on lists and vectors");
        }

        return shsl_mkerr(shsl_ref_to_nil(), "unreachable");
    })
shsl_defun(shsl_builtin_nthcdr, "nthcdr", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        shsl_fn_assert_argtype_either(0, SHSL_CONS, SHSL_NIL);
        shsl_fn_assert_argtype(1, SHSL_INT);

        return shsl_nthcdr(shsl_fn_arg(0), shsl_int(shsl_fn_arg(1)));
    })

/// BUILTIN STRING FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_str, "str", args, env, {
        (void)env;
        shsl_string_builder sb = {0};
        shsl_vec_foreach(i, arg, args)
            shsl_sb_push_obj(&sb, arg, true);

        if(sb.size == 0)
            shsl_mkstr("");
        shsl_sb_push(&sb, '\0');
        return shsl_mkstr_nocopy(shsl_sb_get(&sb));
    })

/// BUILTIN VECTOR FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_vecget, "vec-get", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype(0, SHSL_VEC);
        shsl_fn_assert_argtype(1, SHSL_INT);

        shsl_ref vec = shsl_fn_arg(0);
        shsl_ref ind = shsl_fn_arg(1);
        if((size_t)shsl_int(ind) >= shsl_vec_length(vec))
            return shsl_mkerr
                (ind, "in funciton vecget: vector out of bounds read!");
        return shsl_vec_get(vec, (size_t)shsl_int(ind));
    })
shsl_defun(shsl_builtin_vecset, "vec-set!", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype(0, SHSL_VEC);
        shsl_fn_assert_argtype(1, SHSL_INT);

        shsl_ref vec = shsl_fn_arg(0);
        shsl_ref ind = shsl_fn_arg(1);
        if((size_t)shsl_int(ind) >= shsl_vec_length(vec))
            return shsl_mkerr
                (ind, "in function vecset: vector out of bounds write!");

        return vec;
    })
shsl_defun(shsl_builtin_vecpush, "vec-push!", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype(1, SHSL_VEC);

        shsl_ref vec = shsl_fn_arg(1);
        shsl_vec_push(vec, shsl_fn_arg(0));
        return vec;
    })
shsl_defun(shsl_builtin_veclen, "vec-len", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        shsl_fn_assert_argtype(0, SHSL_VEC);

        return shsl_mkint((long)shsl_vec_length(shsl_fn_arg(0)));
    })
shsl_defun(shsl_builtin_veccat, "vec-cat", args, env, {
        (void)env;
        shsl_vec_foreach(i, arg, args)
            shsl_fn_assert_argtype(i, SHSL_VEC);

        shsl_ref res = shsl_mkvec(0);
        shsl_vec_foreach(i, arg, args) {
            shsl_vec_foreach(j, elt, arg) {
                shsl_vec_push(res, elt);
            }
        }
        return res;
    })
shsl_defun(shsl_builtin_vecslice, "vec-slice", args, env, {
        (void)env;
        shsl_fn_assert_argslen(>= 2);
        shsl_fn_assert_argslen(<= 3);
        shsl_fn_assert_argtype(0, SHSL_VEC);
        shsl_fn_assert_argtype(1, SHSL_INT);
        if(shsl_fn_argslen() == 3)
            shsl_fn_assert_argtype(2, SHSL_INT);

        shsl_ref v = shsl_fn_arg(0);
        ssize_t start_s = shsl_int(shsl_fn_arg(1));
        ssize_t end_s = shsl_fn_argslen()==3
            ?shsl_int(shsl_fn_arg(2))
            :(ssize_t)shsl_vec_length(v);

        if(start_s < 0)
            return shsl_mkerr
                (shsl_fn_arg(1),
                 "cannot slice into vector with negative start index");
        if(shsl_fn_argslen()==3 && end_s < 0)
            return shsl_mkerr
                (shsl_fn_arg(2),
                 "cannot slice into vector with negative end index!");

        size_t start = (size_t)start_s;
        size_t end = (size_t)end_s;
        return shsl_vec_slice(v, start, end);
    })

/// BUILTIN MAP FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_mapget, "map-get", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_fn_assert_argtype(0, SHSL_MAP);
        // TODO: assert key is hashable

        // TODO: this smells of leak
        return shsl_map_get(shsl_fn_arg(0), shsl_fn_arg(1));
    })
shsl_defun(shsl_builtin_mapset, "map-set!", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 3);
        shsl_fn_assert_argtype(0, SHSL_MAP);

        shsl_map_set(shsl_fn_arg(0), shsl_fn_arg(1), shsl_fn_arg(2));
        return shsl_fn_arg(0);
    })

/// COLLECTION CONVERSION FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_vec_to_list, "vec->list", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        shsl_fn_assert_argtype(0, SHSL_VEC);

        shsl_cb cb = shsl_cb_make(SHSL_CB_LIST);
        shsl_vec_foreach(i, elt, shsl_fn_arg(0))
            shsl_cb_add(&cb, elt);
        return shsl_cb_get(cb);
    })
shsl_defun(shsl_builtin_list_to_vec, "list->vec", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        shsl_fn_assert_argtype_either(0, SHSL_CONS, SHSL_NIL);

        shsl_cb cb = shsl_cb_make(SHSL_CB_VEC);
        for(shsl_ref i = shsl_fn_arg(0); shsl_is_cons(i); i = shsl_cdr(i))
            shsl_cb_add(&cb, shsl_car(i));
        return shsl_cb_get(cb);
    })
/// COLLECTION GENERATION VARIADIC FUNCTIONS DEFINITIONS
// note for these two functions
// functions already receive arguments as a parameter vector
shsl_defun(shsl_builtin_vec, "vec", args, env, {
        (void)env;
        return args;
    })
shsl_defun(shsl_builtin_list, "list", args, env, {
        (void)env;
        shsl_cb cb = shsl_cb_make(SHSL_CB_LIST);
        shsl_vec_foreach(i, arg, args)
            shsl_cb_add(&cb, arg);
        return shsl_cb_get(cb);
    })

/// GENERIC COLLECTION FUNCTION DEFINITIONS
// (these are closer in api to what they actually have in clojure)
// (contains? c k) = can you index into collection c with key k?
// works for both (has key?) vectors (has index?)
shsl_defun(shsl_builtin_contains, "contains?", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_ref col = shsl_fn_arg(0);
        shsl_ref key = shsl_fn_arg(1);
        switch(shsl_type(col)) {
        case SHSL_VEC: {
            if(shsl_type(key) != SHSL_INT)
                return shsl_ref_to_nil();
            int key_i = shsl_int(key);
            return shsl_bool_to_obj(key_i >= 0 &&
                                    (size_t)key_i <= shsl_vec_length(col));
        }
        case SHSL_MAP:
            return shsl_bool_to_obj(shsl_map_index(col, key) > 0);
        default:
            return shsl_mkerr(shsl_ref_to_nil(),
                              "contains?: unrecognized collection type, "
                              "can only operate on maps and vectors");
        }

        return shsl_mkerr(shsl_ref_to_nil(), "unreachable");
    })
shsl_defun(shsl_builtin_count, "count", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        shsl_ref col = shsl_fn_arg(0);
        switch(shsl_type(col)) {
        case SHSL_NIL:
            return shsl_mkint(0);
        case SHSL_STR:
            return shsl_mkint(strlen(col.ptr->str));
        case SHSL_VEC:
            return shsl_mkint(shsl_vec_length(col));
        case SHSL_MAP:
            return shsl_mkint(col.ptr->map.size);
        case SHSL_CONS: {
            ssize_t i = shsl_list_length(col);
            if(i>0)
                return shsl_mkint(i);
            return shsl_mkerr
                (shsl_mkint(i),
                 "count:collection an improper list, cannot get count");
        }
        default:
            return shsl_mkerr (col, "cannot get count of object");
        }
        return shsl_mkerr(shsl_ref_to_nil(), "unreachable");
    })

// could have been done in a more stdlib fashion
// but I'll wait for multimethods for that
shsl_defun(shsl_builtin_get, "get", args, env, {
        (void)env;
        shsl_fn_assert_argslen(>= 2);
        shsl_fn_assert_argslen(<= 3);
        shsl_fn_assert_argtype_either(0, SHSL_VEC, SHSL_MAP);

        shsl_ref col = shsl_fn_arg(0);
        shsl_ref ind = shsl_fn_arg(1);
        shsl_ref def = shsl_vec_length(args)==3
            ?shsl_vec_get(args, 2)
            :shsl_ref_to_nil();
        if(shsl_type(col) == SHSL_VEC) {
            if(shsl_type(ind) != SHSL_INT)
                return shsl_mkerr
                    (ind, "cannot index into vector with non integer index!");
            ssize_t is = shsl_int(ind);
            if(is<0)
                return shsl_mkerr
                    (ind, "cannot index into vector with negative index!");
            size_t i = (size_t)is;
            if(i>=shsl_vec_length(col))
                return shsl_mkerr
                    (ind, "index out of bounds, tried accessing element at "
                     "index %zu in an array of length %zu!",
                     i, shsl_vec_length(col));
            return shsl_vec_get(shsl_fn_arg(0), i);
        }
        if(shsl_type(col) == SHSL_MAP) {
            ssize_t i = shsl_map_index(col, ind);
            if(i<0)
                return def;
            return col.ptr->map.buf[i].v;
        }
        return shsl_mkerr
            (shsl_ref_to_nil(), "this bit was supposed to be unreachable");
    })

/// SHSL BUILTIN OTHER FUNCTIONS DEFINITIONS
shsl_defun(shsl_builtin_equal, "=", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        return shsl_bool_to_obj(shsl_eq(shsl_fn_arg(0), shsl_fn_arg(1)));
    })
shsl_defun(shsl_builtin_nequal, "!=", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 2);
        return shsl_bool_to_obj(!shsl_eq(shsl_fn_arg(0), shsl_fn_arg(1)));
    })

shsl_defun(shsl_builtin_pr, "pr", args, env, {
        (void)env;
        shsl_vec_foreach(i, elt, args) {
            shsl_fpr(elt, stdout);
            if(i+1 != shsl_vec_length(args))
                fputc(' ', stdout);
        }
        if(shsl_fn_argslen() == 0)
            return shsl_ref_to_nil();
        return shsl_fn_arg(0);
    })
shsl_defun(shsl_builtin_prn, "prn", args, env, {
        shsl_builtin_pr(args, env);
        fputc('\n', stdout);
        if(shsl_fn_argslen() == 0)
            return shsl_ref_to_nil();
        return shsl_fn_arg(0);
    })
shsl_defun(shsl_builtin_print, "print", args, env, {
        (void)env;
        shsl_vec_foreach(i, elt, args) {
            shsl_fprint(elt, stdout);
            if(i+1 != shsl_vec_length(args))
                fputc(' ', stdout);
        }
        if(shsl_fn_argslen() == 0)
            return shsl_ref_to_nil();
        return shsl_fn_arg(0);
    })
shsl_defun(shsl_builtin_println, "println", args, env, {
        shsl_builtin_print(args, env);
        fputc('\n', stdout);
        if(shsl_fn_argslen() == 0)
            return shsl_ref_to_nil();
        return shsl_fn_arg(0);
    })

shsl_defun(shsl_builtin_fnenv, "fnenv", args, env, {
        (void)env;
        shsl_fn_assert_argslen(== 1);
        shsl_fn_assert(shsl_is_fn(shsl_fn_arg(0)));

        return shsl_fn_env(shsl_fn_arg(0));
    })

shsl_ref shsl_exec(shsl_ref command) {
    assert(shsl_is_vec(command));
    size_t size = shsl_vec_length(command);
    if(size == 0)
        return shsl_mkerr(shsl_ref_to_nil(),
                          "in function exec: no arguments were provided!");

    shsl_vec_foreach(i, command_elt, command)
        if(!shsl_is_str(command_elt))
            return shsl_mkerr(command,
                              "provided command must be a string vector but " 
                              "element at position %zu is not a string, cannot "
                              "run this as a command", i);

    // child program name
    const char* const child_prog = shsl_vec_get(command, 0).ptr->str;

    // child program args
    char* child_args[size+1];
    for(size_t i = 0; i<size; ++i)
        child_args[i] = shsl_vec_get(command, i).ptr->str;
    // arguments must be a null terminated vector for the execvp function
    child_args[size] = (char*)NULL;
    // fork and exec
    pid_t pid = fork();
    if(pid) {
        // we'll overwrite this int and the initial value isn't that important
        // we just set it to a random -1 avoid having any uninitialized data
        int child_return_status = -1;
        waitpid(pid, &child_return_status, 0);
        if(child_return_status == 0)
            return shsl_mksym("t");
        return shsl_mkerr(shsl_mkint(child_return_status),
                          "child process returned with error status");
    } else {
        execvp(child_prog, child_args);
        return shsl_mkerr(shsl_mkint(errno),
                          "fucked something up in the child process");
    }
}

shsl_defun(shsl_builtin_exec, "exec", args, env, {
        // TODO: guess who ain't supporting windows rn :|
        (void)env;
        shsl_fn_assert_argslen(>= 1);
        return shsl_exec(args);
    })

shsl_defun(shsl_builtin_exec_vec, "exec-vec", args, env, {
        // TODO: guess who ain't supporting windows rn :|
        (void)env;
        shsl_fn_assert_argslen(== 1);
        shsl_fn_assert_argtype(0, SHSL_VEC);
        return shsl_exec(shsl_fn_arg(0));
    })

// FIXME: very temporary, these should be macros, functions can't short circuit
shsl_defun(shsl_builtin_or, "or", args, env, {
        (void)env;
        bool any_true = false;
        shsl_vec_foreach(i, arg, args)
            if(shsl_is_truthy(arg)) {
                any_true=true;
                break;
            }

        return shsl_bool_to_obj(any_true);
    })

shsl_defun(shsl_builtin_and, "and", args, env, {
        (void)env;
        bool any_false = false;
        shsl_vec_foreach(i, arg, args)
            if(!shsl_is_truthy(arg)) {
                any_false=true;
                break;
            }

        return shsl_bool_to_obj(!any_false);
    })

shsl_defun(shsl_builtin_macroexpand_1, "macroexpand-1", args, env, {
        shsl_fn_assert_argslen(==1);
        shsl_fn_assert_argtype_either(0, SHSL_NIL, SHSL_CONS);
        return shsl_macroexpand_1(shsl_fn_arg(0), env);
    })

shsl_defun(shsl_builtin_exit, "exit", args, env, {
        // this function's a bit weird on the error management
        // if the user provides incorrect arguments to exit, we're gonna assume
        // they wanted to exit, so we're gonna exit anyway
        // but we'll do so with an error
        (void)env;
        if(shsl_fn_argslen() != 1)
            exit(EXIT_FAILURE);
        if(shsl_type(shsl_fn_arg(0)) != SHSL_INT)
            exit(EXIT_FAILURE);

        int i = shsl_int(shsl_fn_arg(0));
        exit(i);
        return shsl_mkerr(shsl_ref_to_nil(), "unreachable");
    })

// miscellaneous functions
// load file
shsl_defun(shsl_builtin_load, "load", args, env, {
        shsl_fn_assert_argslen(==1);
        shsl_fn_assert_argtype(0, SHSL_STR);
        const char* filepath = shsl_fn_arg(0).ptr->str;

        return shsl_eval_file(filepath, env);
    })
// generate unique symbol, (for macros)
// TODO: currently hardcoded max number of tries at 9999
// this is already overkill, but I'd like it to be like, mildly parameterized
shsl_defun(shsl_builtin_gensym, "gensym", args, env, {
        char* base;
        if(shsl_fn_argslen() == 0)
            base = "gensym-";
        else if (shsl_fn_argslen() == 1) {
            shsl_ref gsname = shsl_fn_arg(0);
            if(shsl_type(gsname) != SHSL_STR)
                return shsl_mkerr
                    (gsname, "when given one argument gensym expects "
                     "the first argument to be of type string");
            // shsl_cat_strs heap allocates, remember this fact
            // for when we get to the teardown of this functions
            base = shsl_cat_strs("gensym-", gsname.ptr->str, "-");
        }
        else
            return shsl_mkerr
                (args,
                 "gensym expects 0 or 1 arguments, it cannot take "
                 "%zu items", (size_t)shsl_fn_argslen());
        
        const size_t base_len = strlen(base);
        const char* numstr = "----";
        char* symname = shsl_cat_strs(base, numstr);
        shsl_ref sym = shsl_mksym_nocopy(symname);
        for(size_t try = 0; try<10000; try++) {
            sprintf(symname+base_len, "%zu", try);
            if(!shsl_env_has(env, sym)) {
                if(shsl_fn_argslen() == 1) free(base);
                return sym;
            }
        }
        if(shsl_fn_argslen() == 1) free(base);
        return shsl_mkerr(sym, "could not generate symbol, ran out of tries");
    })

// environment creation
shsl_ref shsl_env_mkempty(void) {
    return shsl_mkcons(shsl_mkmap(20), // arbitrary initial size
                       shsl_ref_to_nil());
}

// when initialized functions are often passed a ref to the environment they
// were defined in, this can lead to circular refrences.
// and we're using refcounting so that's a bit of a fucking problem
void shsl_env_fix_circular_fn_refs(shsl_ref env) {
    shsl_ref top_frame = shsl_car(env);
    assert(shsl_type(top_frame) == SHSL_MAP);

    for(size_t i = 0; i<top_frame.ptr->map.size; ++i) {
        shsl_ref v = top_frame.ptr->map.buf[i].v;
        if(shsl_is_fn(v) && shsl_fn_env(v).ptr == env.ptr) {
            shsl_fn_env_mark_weak(v);
        }
    }
}

shsl_ref shsl_env_add_initial_definitions(shsl_ref env) {
    shsl_env_def(env, shsl_mksym("nil"), shsl_ref_to_nil());
    shsl_ref t = shsl_mksym("t");
    shsl_env_def(env, t, t); // t is self evaluating

    // arithmetic operations 
    shsl_env_def(env, shsl_mksym("+"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_add));
    shsl_env_def(env, shsl_mksym("-"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_sub));
    shsl_env_def(env, shsl_mksym("*"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_mul));
    shsl_env_def(env, shsl_mksym("/"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_div));

    shsl_env_def(env, shsl_mksym(">"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_gt));
    shsl_env_def(env, shsl_mksym("<"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_lt));
    shsl_env_def(env, shsl_mksym(">="),
                 shsl_mkbuiltin_fn(env, shsl_builtin_ge));
    shsl_env_def(env, shsl_mksym("<="),
                 shsl_mkbuiltin_fn(env, shsl_builtin_le));

    // data predicates
    shsl_env_def(env, shsl_mksym("null?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isnil));
    shsl_env_def(env, shsl_mksym("nil?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isnil));
    shsl_env_def(env, shsl_mksym("int?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isint));
    shsl_env_def(env, shsl_mksym("real?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isreal));
    shsl_env_def(env, shsl_mksym("sym?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_issym));
    shsl_env_def(env, shsl_mksym("str?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isstr));
    shsl_env_def(env, shsl_mksym("err?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_iserr));
    shsl_env_def(env, shsl_mksym("cons?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_iscons));
    shsl_env_def(env, shsl_mksym("vec?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isvec));
    shsl_env_def(env, shsl_mksym("map?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_ismap));
    shsl_env_def(env, shsl_mksym("list?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_islist));

    shsl_env_def(env, shsl_mksym("fn?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_isfn));
    shsl_env_def(env, shsl_mksym("macro?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_ismacro));
    shsl_env_def(env, shsl_mksym("not"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_not));

    // list operations
    // TODO: proper? (tortoise and hare and shit)
    shsl_env_def(env, shsl_mksym("list"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_list));
    shsl_env_def(env, shsl_mksym("cons"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_cons));
    shsl_env_def(env, shsl_mksym("car"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_car));
    shsl_env_def(env, shsl_mksym("cdr"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_cdr));
    shsl_env_def(env, shsl_mksym("nth"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_nth));
    shsl_env_def(env, shsl_mksym("nthcdr"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_nthcdr));

    // vector operations
    shsl_env_def(env, shsl_mksym("vec"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vec));
    shsl_env_def(env, shsl_mksym("vec-get"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vecget));
    shsl_env_def(env, shsl_mksym("vec-set!"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vecset));
    shsl_env_def(env, shsl_mksym("vec-len"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_veclen));
    shsl_env_def(env, shsl_mksym("vec-push!"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vecpush));
    shsl_env_def(env, shsl_mksym("vec-cat"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_veccat));
    shsl_env_def(env, shsl_mksym("vec-slice"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vecslice));

    // map operations
    shsl_env_def(env, shsl_mksym("map-get"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_mapget));
    shsl_env_def(env, shsl_mksym("map-set!"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_mapset));

    // string operations(?)
    shsl_env_def(env, shsl_mksym("str"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_str));

    // collection operations
    shsl_env_def(env, shsl_mksym("get"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_get));
    shsl_env_def(env, shsl_mksym("count"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_count));
    shsl_env_def(env, shsl_mksym("contains?"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_contains));
    // TODO: also map, filter, reduce

    // collection conversion operations
    shsl_env_def(env, shsl_mksym("vec->list"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vec_to_list));
    shsl_env_def(env, shsl_mksym("list->vec"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_vec_to_list));

    // error functions
    shsl_env_def(env, shsl_mksym("err"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_err));

    // miscellaneous functions
    shsl_env_def(env, shsl_mksym("="),
                 shsl_mkbuiltin_fn(env, shsl_builtin_equal));
    shsl_env_def(env, shsl_mksym("!="),
                 shsl_mkbuiltin_fn(env, shsl_builtin_nequal));
    shsl_env_def(env, shsl_mksym("pr"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_pr));
    shsl_env_def(env, shsl_mksym("prn"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_prn));
    shsl_env_def(env, shsl_mksym("print"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_print));
    shsl_env_def(env, shsl_mksym("println"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_println));
    // macro oriented functions
    shsl_env_def(env, shsl_mksym("and"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_and));
    shsl_env_def(env, shsl_mksym("or"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_or));
    shsl_env_def(env, shsl_mksym("gensym"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_gensym));
    shsl_env_def(env, shsl_mksym("macroexpand-1"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_macroexpand_1));
    // introspection functions
    shsl_env_def(env, shsl_mksym("fnenv"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_fnenv));

    // systemy functions
    shsl_env_def(env, shsl_mksym("exec"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_exec));
    shsl_env_def(env, shsl_mksym("exec-vec"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_exec_vec));
    shsl_env_def(env, shsl_mksym("exit"),
                 shsl_mkbuiltin_fn(env, shsl_builtin_exit));

    // TODO: ctypes equivalent
    return env;
}


shsl_ref shsl_env_eval_stdlib(shsl_ref env) {
    // shsl is self contained in one big c file so...
    // we don't put the standard library in a separate file either
    // if opengl tutorials can do this then by golly so can we

    // TODO: add this fabled standard library
    static const char* stdlib =
        "";
    (void)stdlib;
    (void)env;
    return shsl_mkerr(shsl_ref_to_nil(), "todo");
}

shsl_ref shsl_env_mkinitial(void) {
    shsl_ref env = shsl_ref_add(shsl_env_mkempty());
    shsl_env_add_initial_definitions(env);
    return env;
}

//// PRINT DEBUGGING DEFINITIONS
//// ----------------------------------------------------------------------------
/// SERIALIZATION DEFINITIONS
#define SHSL_MAX_ATOM_SERIALIZATION_LENGTH 256
void shsl_sb_push_obj(shsl_string_builder* sb, shsl_ref obj, bool pretty) {
    // only difference between pretty printing and dumping is putting the
    // quotes around strings, bit of a waste to write two functions that do
    // everything the same but two lines, so I'm making one function with a switch
    // to either print or dump
    // (ie: to either put the quotes or not if it sees a string)
    static char buf[SHSL_MAX_ATOM_SERIALIZATION_LENGTH];
    static int written;

    switch(shsl_type(obj)) {
    case SHSL_INT:
        written = sprintf(buf, "%ld", obj.ptr->i);
        shsl_sb_push_sized_str(sb, buf, written);
        break;
    case SHSL_REAL:
        written = sprintf(buf, "%f", obj.ptr->r);
        shsl_sb_push_sized_str(sb, buf, written);
        break;
    case SHSL_STR:
        if(!pretty) {
            // dump to be read by reader
            // handle all escapes and display them as printable ascii
            shsl_sb_push(sb, '"');
            for(const char* c = obj.ptr->str; *c; c++) {
                if(is_escape_char(*c))
                    shsl_sb_push_nullt_str(sb, to_string_representation(*c));
                else
                    shsl_sb_push(sb, *c);
            }
            shsl_sb_push(sb, '"');
        }
        else {
            // I'm a pretty princess
            // and displaying escape sequences is not pretty
            shsl_sb_push_nullt_str(sb, obj.ptr->str);
        }
        break;
    case SHSL_SYM:
        shsl_sb_push_nullt_str(sb, obj.ptr->sym.name.ptr->str);
        break;
    case SHSL_NIL:
        shsl_sb_push_nullt_str(sb, "nil");
        break;
    case SHSL_ERR:
        shsl_sb_push_nullt_str(sb, "(err ");
        shsl_sb_push_obj(sb, obj.ptr->err.msg, false);
        shsl_sb_push_nullt_str(sb, " ");
        shsl_sb_push_obj(sb, obj.ptr->err.data, false);
        shsl_sb_push_nullt_str(sb, ")");
        break;
    case SHSL_CONS:
        if(shsl_is_proper_list(obj)) {
            shsl_sb_push(sb, '(');
            shsl_ref iter = obj;
            while(shsl_is_cons(iter)) {
                shsl_sb_push_obj(sb, iter.ptr->cons.car, false);
                if(shsl_is_cons(iter.ptr->cons.cdr))
                    shsl_sb_push(sb, ' ');
                iter = iter.ptr->cons.cdr;
            }
            shsl_sb_push(sb, ')');
        }
        else {
            shsl_sb_push_nullt_str(sb, "( ");
            shsl_sb_push_obj(sb, obj.ptr->cons.car, false);
            shsl_sb_push_nullt_str(sb, " . ");
            shsl_sb_push_obj(sb, obj.ptr->cons.cdr, false);
            shsl_sb_push_nullt_str(sb, ")");
        }
        break;
    case SHSL_VEC:
        shsl_sb_push(sb, '[');
        shsl_vec_foreach(i, elt, obj) {
            shsl_sb_push_obj(sb, elt, false);
            if(i != shsl_vec_length(obj)-1)
                shsl_sb_push(sb, ' ');
        }
        shsl_sb_push(sb, ']');
        break;
    case SHSL_MAP:
        shsl_sb_push(sb,'{');
        for(size_t i = 0; i<obj.ptr->map.size; ++i) {
            shsl_sb_push_obj(sb, obj.ptr->map.buf[i].k, false);
            shsl_sb_push(sb,' ');
            shsl_sb_push_obj(sb, obj.ptr->map.buf[i].v, false);
            if(i != (obj.ptr->map.size-1))
                shsl_sb_push(sb,' ');
        }
        shsl_sb_push(sb,'}');
        break;
        // TODO: I ain't making the reader understand this shit
    case SHSL_BUILTIN_FN:
        sprintf(buf, "SHSL_BUILTIN_FN_%p", (void*)(obj.ptr));
        shsl_sb_push_nullt_str(sb, buf);
        break;
    case SHSL_USER_FN:
        sprintf(buf, "SHSL_USER_FN_%p", (void*)(obj.ptr));
        shsl_sb_push_nullt_str(sb, buf);
        break;
    case SHSL_BUILTIN_MACRO:
        sprintf(buf, "SHSL_BUILTIN_MACRO_%p", (void*)(obj.ptr));
        shsl_sb_push_nullt_str(sb, buf);
        break;
    case SHSL_USER_MACRO:
        sprintf(buf, "SHSL_USER_MACRO_%p", (void*)(obj.ptr));
        shsl_sb_push_nullt_str(sb, buf);
        break;
    };
}
char* shsl_to_str(shsl_ref obj) {
    shsl_string_builder sb = {0};
    shsl_sb_push_obj(&sb, obj, true);
    shsl_sb_push(&sb, '\0');
    return shsl_sb_get(&sb);
}
char* shsl_dump(shsl_ref obj) {
    shsl_string_builder sb = {0};
    shsl_sb_push_obj(&sb, obj, false);
    shsl_sb_push(&sb, '\0');
    return shsl_sb_get(&sb);
}

// you're expected to push the null terminator in here before calling sb_get
char* shsl_sb_get(shsl_string_builder* sb) {
    // also once we call sb_get it's expected you're done with the string
    // ie: done with building/appending to it
    // so we don't need the preallocated tail anymore
    sb->buf = (char*)realloc(sb->buf, sb->size);
    sb->capacity = sb->size;
    return sb->buf;
}

/// PRINTING DEFINITIONS
void shsl_dbg_fputtok(const shsl_token* tok, FILE* stream) {
    fputs("token ", stream);
    switch(tok->type) {
    case SHSL_TOK_NIL:
        fputs("SHSL_TOK_NIL: ", stream);
        break;
    case SHSL_TOK_INTEGER:
        fputs("SHSL_TOK_INTEGER: ", stream);
        break;
    case SHSL_TOK_REAL:
        fputs("SHSL_TOK_REAL: ", stream);
        break;
    case SHSL_TOK_STRING:
        fputs("SHSL_TOK_STRING: ", stream);
        break;
    case SHSL_TOK_SYMBOL:
        fputs("SHSL_TOK_SYMBOL: ", stream);
        break;
    case SHSL_TOK_QUOTE:
        fputs("SHSL_TOK_QUOTE: ", stream);
        break;
    case SHSL_TOK_QUASIQUOTE:
        fputs("SHSL_TOK_QUASIQUOTE: ", stream);
        break;
    case SHSL_TOK_COMMA:
        fputs("SHSL_TOK_COMMA: ", stream);
        break;
    case SHSL_TOK_OPEN_PAREN:
        fputs("SHSL_TOK_OPEN_PAREN: ", stream);
        break;
    case SHSL_TOK_CLOSE_PAREN:
        fputs("SHSL_TOK_CLOSE_PAREN: ", stream);
        break;
    case SHSL_TOK_OPEN_SQUARE:
        fputs("SHSL_TOK_OPEN_SQUARE: ", stream);
        break;
    case SHSL_TOK_CLOSE_SQUARE:
        fputs("SHSL_TOK_CLOSE_SQUARE: ", stream);
        break;
    case SHSL_TOK_OPEN_CURLY:
        fputs("SHSL_TOK_OPEN_CURLY: ", stream);
        break;
    case SHSL_TOK_CLOSE_CURLY:
        fputs("SHSL_TOK_CLOSE_CURLY: ", stream);
        break;
    case SHSL_TOK_EOF:
        fputs("SHSL_TOK_EOF: ", stream);
        break;
    case SHSL_TOK_ERROR:
        fputs("SHSL_TOK_ERROR: ", stream);
    };
    shsl_fpr(tok->ref, stream);
}
void shsl_fpr(const shsl_ref ref, FILE* stream) {
    char* c = shsl_dump(ref);
    fputs(c, stream);
    free(c);
}
void shsl_fprint(const shsl_ref ref, FILE* stream) {
    char* c = shsl_to_str(ref);
    fputs(c, stream);
    free(c);
}

//// USER FACING FUNCTIONS DEFINITIONS
//// ----------------------------------------------------------------------------

/// SHSL SUBROUTINES FOR MAIN DECLARATIONS
shsl_ref shsl_eval_str(char* c, shsl_ref env) {
    shsl_ref curr = shsl_ref_to_nil();
    while(true) {
        parser_pair p = parse_off(c);
        c = p.remaining;
        // eof in parse_off is signaled by returning (parser_pair){0}
        // so, if(!c), it means we reached eof
        if(!c) {
            if(p.ref.ptr)
                shsl_free(p.ref);
            break;
        }

        // TODO: make form_to_expr accept an environment so we can
        // do macro expansion during form "compilation"?
        shsl_ref_add(p.ref);
        shsl_expr* expr = shsl_form_to_expr(p.ref, env);
        shsl_ref_drop(p.ref);

        shsl_ref_set(&curr, shsl_eval(expr, env));
        shsl_expr_free(expr);
    }

    // setting curr to the result of eval creates an artificial reference to it
    // which can cause leakage
    // but we can't use shsl_ref_drop() on curr because this may be the only ref
    // pointing to the result of eval
    // fresh values out of eval are valid references with a ref count of 0
    if(!shsl_is_nil(curr))curr.ptr->ref_count--;
    return curr;
}
char* shsl_read_file(const char *path) {
    // reads entire contents file at path in a null terminated string
    FILE *f = fopen(path, "rb");
    if (f == NULL || fseek(f, 0, SEEK_END) < 0) {
        fprintf(stderr, "Could not read file %s: %s", path, strerror(errno));
        if(f) fclose(f);
        return NULL;
    }

    long long m = 0;
#ifndef _WIN32
    m = ftell(f);
#else
    m = _telli64(_fileno(f));
#endif
    if (m < 0 || fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "Could not read file %s: %s", path, strerror(errno));
        if(f) fclose(f);
        return NULL;
    }

    char* contents = (char*)calloc(m+1, sizeof(char));
    fread((void*)contents, m, 1, f);
    contents[m] = '\0';

    if (ferror(f)) {
        fprintf(stderr, "Could not read file %s:", path);
        if(f) fclose(f);
        free(contents);
        return NULL;
    }

    if(f) fclose(f);
    //*file_size = m;
    return contents;
}
shsl_ref shsl_eval_file(const char* filepath, shsl_ref env) {
    char* c = shsl_read_file(filepath);
    if(!c)
        return shsl_mkerr(shsl_mkstr(filepath),
                          "could not read contents of file to evaluate it");

    char* anchor = c;
    // skip shebang if present
    if(c[0] == '#' && c[1] == '!') {
        // have to handle special case in which shebang is the only thing
        // in file so it doesn't end with newline, but with end of file
        // (since we read everything in a null terminated string this is
        // marked by '\0')
        while(*c != '\n' && *c != '\0') c++;
        if(*c == '\n') c++;
    }
    shsl_ref ref = shsl_eval_str(c, env);
    free(anchor);
    return ref;
}
void shsl_repl(shsl_ref env) {
    char* line = NULL;
    size_t linelen;
    fprintf(stdout, "shsl> ");
    while(getline(&line, &linelen, stdin) != -1) {
        shsl_ref ref = shsl_eval_str(line, env);
        fprintf(stdout, "=> "); shsl_fprint(ref, stdout); fputc('\n', stdout);
        free(line);
        line = NULL;
        fprintf(stdout, "\nshsl> ");
    }
    // once getline returns -1 it means eof
    // was it good eof or bad eof? let's ask errno!
    if(errno)
        printf("\nwell fuck me then :/\n");
    else
        printf("\nbye bye :)\n");
}

/// SHSL MAIN DEFINTIONS
void shsl_usage(const char* name, bool print_extra, FILE* stream) {
    static char* msg =
        "usage\n"
        "%s [ | [<flag> [<flag operand>]]+ | <file> <file args>* ]\n"
        "* %s by itself starts a repl\n"
        "\n"
        "* %s <file> <file args> runs file <file> passing it <file args>\n"
        "  in the argv variable\n"
        "\n"
        "* %s [<flag> [<flag operand>]]+ the flags can be either:\n"
        "   -e <str>  : evaluates string and prints result\n"
        "   -f <file> : reads and evaluates everything in file\n"
        "   -r starts a read eval print loop (repl)\n"
        "   -h prints help message and exits\n";
    static char* extra =
        "\n"
        "  multiple <flag> [<flag operand>]s can be put one after the other,\n"
        "  in which case they will be evaluated in order\n"
        "\n"
        "  for instance:\n"
        "  %s -e \"(def a 10)\" -f file.shsl -r \n"
        "  will define a to be 10, run file.shsl with a bound to 10,\n"
        "  and after all that, in the vm where the previous two flags were run,\n"
        "  start a repl\n";
    fprintf(stream, msg, name, name, name, name);
    if(print_extra)
        fprintf(stream, extra, name);
}
void shsl_die(int exit_with, const char* errmsg, ...) {
    va_list args;
    va_start(args, errmsg);
    vfprintf(stderr, errmsg, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(exit_with);
}
int shsl_main(int argc, char** argv) {
    // init
    shsl_ref env = shsl_env_mkinitial();
    shsl_env_def(env, shsl_mksym("progname"), shsl_mkstr(argv[0]));

    // no args, default behaviour just start a repl
    if(argc == 1) {
        shsl_repl(env);
        goto teardown;
    }

    // handle ./shsl [file] [args] case (mainly for shebangs)
    // and ease in passing argv to shsl files
    if(argv[1][0] != '-') { // if first arg is not a flag
        shsl_ref shsl_argv = shsl_mkvec(1);
        for(int i = 2; i<argc; ++i) {
            shsl_vec_push(shsl_argv, shsl_mkstr(argv[i]));
        }
        shsl_env_def(env, shsl_mksym("argv"), shsl_argv);
        shsl_ref_drop(shsl_ref_add(shsl_eval_file(argv[1], env)));
        goto teardown;
    }

    shsl_env_def(env, shsl_mksym("argv"), shsl_mkvec(0));

    // iterate and execute all
    int i = 1;
    while(i<argc) {
        if(strcmp(argv[i], "-e") == 0) {
            if(i+1 >= argc)
                shsl_die(1, "-e flag specified without any code after the flag!");
            char* line = argv[i+1];
            shsl_ref ref = shsl_eval_str(line, env);
            shsl_ref_add(ref);
            shsl_fprint(ref, stdout);
            shsl_ref_drop(ref);
            puts("");
            i+=2;
        }
        else if(strcmp(argv[i], "-f") == 0) {
            if(i+1 >= argc)
                shsl_die(1, "-f flag specified without any file name after the flag!");
            char* filename = argv[i+1];
            shsl_ref_drop(shsl_ref_add(shsl_eval_file(filename, env)));
            i+=2;
        }
        else if(strcmp(argv[i], "-r") == 0) {
            shsl_repl(env);
            i++;
        }
        else if(strcmp(argv[i], "-h") == 0) {
            shsl_usage(argv[0], true, stdout);
            return 0;
        }
        else {
            fprintf(stderr, "'%s' unrecognized flag!!\n", argv[i]);
            shsl_usage(argv[0], false, stderr);
            return 1;
        }
    }
 teardown:
    shsl_ref_drop(env);
    return 0;
}

#endif // SHSL_IMPLEMENTATION
